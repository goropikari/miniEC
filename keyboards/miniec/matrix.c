/* Copyright 2020 sekigon-gonnoc
 * Copyright 2023 goropikari
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "matrix.h"
#include "config.h"
#include "debounce.h"
#include "debug.h"
#include "quantum.h"
#include "split_util.h"
#include "transport.h"

#include "debug.h"
#include "matrix.h"
#include "print.h"
#include "quantum.h"
#include "split_util.h"
#include "util.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <util/delay.h>

#include "analog.h"

#ifdef MINIEC_WEB_TUNING
#include "eeconfig.h"
#ifdef VIAL_ENABLE
#include "via.h"
#else
#include "raw_hid.h"
#endif
#endif

#define ROWS_PER_HAND (MATRIX_ROWS / 2)
#define KEY_NUM (MATRIX_COLS * ROWS_PER_HAND)
#define KEY2CR(K, C, R)                                                        \
  {                                                                            \
    (C) = (K) % MATRIX_COLS;                                                   \
    (R) = (K) / MATRIX_COLS;                                                   \
  }
#define CR2KEY(C, R, K)                                                        \
  {                                                                            \
    (K) = ((C) % MATRIX_COLS) + (((R) % ROWS_PER_HAND) * MATRIX_COLS);         \
  }
#define VAL2THRESHOLD(VAL) (((VAL) >= 1024) ? 255 : (VAL / 4))

#define PI ((float)3.1415926535)
#define CALIBRATE_BUF 5
#define CALIBRATE_INI 20
#define CALIBRATE_FREQ_MS 10000
#define CALIBRATE_STABLE_MARGIN 2
#define CALIBRATE_STABLE_COUNT 10
#define CALIBRATE_MAX_ROUNDS 100
#define DEFAULT_THRESHOLD_DOWN 90
#define DEFAULT_THRESHOLD_UP 25
#define DEFAULT_FILTER_Q (1.0 / sqrt(2.0))
#define DEFAULT_SAMPLE_HZ 150.0
#define DEFAULT_LOWPASS_HZ 1.0

uint32_t timer_1ms;
uint32_t last_timer;

static uint16_t ecsm_sw_value[ROWS_PER_HAND][MATRIX_COLS];

typedef struct {
  uint8_t threshold;
  uint8_t buf[CALIBRATE_BUF];
  uint8_t idx;
} calibrate_info;
calibrate_info calibrate[KEY_NUM];

#ifdef MINIEC_WEB_TUNING
typedef struct {
  uint8_t press;
  uint8_t release;
} key_offsets_t;

static key_offsets_t key_offsets[KEY_NUM];
_Static_assert(sizeof(key_offsets) == EECONFIG_KB_DATA_SIZE,
               "Wrong EEPROM size");

static bool offsets_valid(uint8_t press, uint8_t release) {
  return release > 0 && release < press && press < UINT8_MAX;
}

static void read_saved_offsets(key_offsets_t *offsets, uint8_t first,
                               uint8_t count) {
  uint8_t length = count * sizeof(*offsets);
  if (eeconfig_read_kb_datablock(offsets, first * sizeof(*offsets), length) !=
      length) {
    memset(offsets, 0, length);
  }
  for (uint8_t i = 0; i < count; ++i) {
    if (!offsets_valid(offsets[i].press, offsets[i].release)) {
      offsets[i] = (key_offsets_t){HIGH_THRESHOLD, LOW_THRESHOLD};
    }
  }
}

static void load_key_offsets(void) {
  read_saved_offsets(key_offsets, 0, KEY_NUM);
}

static bool save_key_offsets(const key_offsets_t *offsets) {
  return eeconfig_update_kb_datablock(offsets, 0, sizeof(key_offsets)) ==
         sizeof(key_offsets);
}

enum {
  TUNING_INFO = 1,
  TUNING_READ,
  TUNING_APPLY,
  TUNING_SAVE,
  TUNING_READ_SAVED,
  TUNING_RESTORE,
};

#define TUNING_PAGE_KEYS (KEY_NUM / 2)
_Static_assert(KEY_NUM % 2 == 0 &&
                   TUNING_PAGE_KEYS * sizeof(key_offsets_t) <= 25,
               "Invalid tuning page size");

static key_offsets_t restore_offsets[KEY_NUM];
static bool restore_pending;
static uint8_t restore_next_id;

static bool restore_page(uint8_t first, uint8_t request_id,
                         const uint8_t *pairs) {
  bool last = first == TUNING_PAGE_KEYS;
  bool expected = restore_pending && request_id == restore_next_id;
  restore_pending = false;
  if (first != 0 && (!last || !expected))
    return false;

  for (uint8_t i = 0; i < TUNING_PAGE_KEYS; ++i) {
    key_offsets_t offsets = {pairs[i * 2], pairs[i * 2 + 1]};
    if (!offsets_valid(offsets.press, offsets.release))
      return false;
    restore_offsets[first + i] = offsets;
  }
  if (!last) {
    restore_pending = true;
    restore_next_id = request_id + 1;
    return true;
  }
  // Do not change live offsets until both validated pages have been saved.
  if (!save_key_offsets(restore_offsets))
    return false;
  memcpy(key_offsets, restore_offsets, sizeof(key_offsets));
  return true;
}

static bool handle_tuning_report(uint8_t *data, uint8_t length) {
  // Header: 'M', 'E', protocol version, command, request ID. Reply: status at
  // byte 5.
  if (length != 32 || data[0] != 'M' || data[1] != 'E' || data[2] != 1) {
    return false;
  }

  const uint8_t key = data[5];
  const uint8_t press = data[6];
  const uint8_t release = data[7];
  bool restore_ok = false;
  if (data[3] == TUNING_RESTORE) {
    restore_ok = restore_page(key, data[4], data + 6);
  } else {
    restore_pending = false;
  }
  memset(data + 5, 0, length - 5);
  switch (data[3]) {
  case TUNING_INFO:
    data[6] = is_keyboard_left() ? 0 : 1;
    data[7] = ROWS_PER_HAND;
    data[8] = MATRIX_COLS;
    data[9] = HIGH_THRESHOLD;
    data[10] = LOW_THRESHOLD;
    data[11] =
        TUNING_PAGE_KEYS; // Nonzero when backup and restore are supported.
    break;
  case TUNING_APPLY:
  case TUNING_READ:
    if (key >= KEY_NUM ||
        (data[3] == TUNING_APPLY && !offsets_valid(press, release))) {
      data[5] = 1;
      break;
    }
    if (data[3] == TUNING_APPLY) {
      key_offsets[key] = (key_offsets_t){press, release};
    }
    data[6] = ecsm_sw_value[key / MATRIX_COLS][key % MATRIX_COLS];
    data[7] = calibrate[key].threshold;
    data[8] = key_offsets[key].press;
    data[9] = key_offsets[key].release;
    break;
  case TUNING_SAVE:
    data[5] = !save_key_offsets(key_offsets);
    break;
  case TUNING_READ_SAVED: {
    if (key != 0 && key != TUNING_PAGE_KEYS) {
      data[5] = 1;
      break;
    }
    key_offsets_t page[TUNING_PAGE_KEYS];
    read_saved_offsets(page, key, TUNING_PAGE_KEYS);
    memcpy(data + 6, page, sizeof(page));
    break;
  }
  case TUNING_RESTORE:
    data[5] = !restore_ok;
    break;
  default:
    data[5] = 1;
    break;
  }
  // VIA may echo a request while Vial is unlocking. Only mark handled replies.
  data[31] = 0xA5;
  return true;
}

#ifdef VIAL_ENABLE
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
  if (!handle_tuning_report(data, length) && length != 0) {
    data[0] = id_unhandled;
  }
}
#else
void raw_hid_receive(uint8_t *data, uint8_t length) {
  if (handle_tuning_report(data, length)) {
    raw_hid_send(data, length);
  }
}
#endif
#endif

// sensing channel definitions
#define S0 0
#define S1 1
#define S2 2
#define S3 3
#define S4 4
#define S5 5
#define S6 6
#define S7 7

// pin connections
const uint8_t row_pins[] = MATRIX_ROW_PINS;
const uint8_t col_channels[] = COL_PINS;
const uint8_t mux_sel_pins[] = MUX_SEL_PINS;

_Static_assert(sizeof(mux_sel_pins) == 3, "invalid MUX_SEL_PINS");

typedef struct {
  float a; // Feed forward coefficient
  float b; // Feedback coefficient

  float input;
  float output;
  uint8_t first;
} filter_info;

filter_info filter[KEY_NUM];

void filter_config(filter_info *f, float fc, float q, float fs) {

  // fc Digital LPF cutoff frequency
  // q  Quality factor (1 / sqrt (2) default)
  // fs Sampling frequency

  float t = 1.0 / fs;
  float c = 1.0 * PI * fc;
  f->b = tan(c * t / 2.0) / (1.0 + tan(c * t / 2.0));
  f->a = (1.0 - tan(c * t / 2.0)) / (1.0 + tan(c * t / 2.0));
  f->first = 1;
}

float filter_process(filter_info *f, float val) {
  float result;
  if (f->first) {
    f->input = val;
    f->output = val;
    f->first = 0;
  }

  result = f->b * val + f->b * f->input + f->a * f->output;
  f->input = val;
  f->output = result;
  return result;
}

static inline void discharge_capacitor(void) {
  gpio_set_pin_output(DISCHARGE_PIN);
}

static inline void charge_capacitor(uint8_t row) {
  gpio_set_pin_input(DISCHARGE_PIN);
  gpio_write_pin_high(row_pins[row]);
}

static inline void clear_all_row_pins(void) {
  for (int row = 0; row < sizeof(row_pins); row++) {
    gpio_write_pin_low(row_pins[row]);
  }
}

static inline void init_mux_sel(void) {
  for (int idx = 0; idx < sizeof(mux_sel_pins); idx++) {
    gpio_set_pin_output(mux_sel_pins[idx]);
  }
}

static inline void select_mux(uint8_t col) {
  uint8_t ch = col_channels[col];
  gpio_write_pin(mux_sel_pins[0], ch & 1);
  gpio_write_pin(mux_sel_pins[1], ch & 2);
  gpio_write_pin(mux_sel_pins[2], ch & 4);
}

static inline void init_row(void) {
  for (int idx = 0; idx < sizeof(row_pins); idx++) {
    gpio_set_pin_output(row_pins[idx]);
    gpio_write_pin_low(row_pins[idx]);
  }
}

// Initialize pins
void ecsm_init(void) {
  // // save config
  // config = *ecsm_config;

  // initialize discharge pin as discharge mode
  gpio_write_pin_low(DISCHARGE_PIN);
  gpio_set_pin_output(DISCHARGE_PIN);

  // set analog reference
  analogReference(ADC_REF_POWER);

  // initialize drive lines
  init_row();

  // initialize multiplexer select pin
  init_mux_sel();

  // set discharge pin to charge mode
  gpio_set_pin_input(DISCHARGE_PIN);
}

// Read key value of key (row, col)
uint16_t ecsm_readkey_raw(uint8_t row, uint8_t col) {
  uint16_t sw_value = 0;

  discharge_capacitor();

  select_mux(col);

  clear_all_row_pins();

  charge_capacitor(row);

  sw_value = analogReadPin(ANALOG_PORT);

  return sw_value;
}

uint16_t ecsm_readkey_raw_k(uint8_t key) {
  uint8_t col, row;
  KEY2CR(key, col, row);
  return ecsm_readkey_raw(row, col);
}

// Update press/release state of key at (row, col)
bool ecsm_update_key(matrix_row_t *current_row, uint8_t row, uint8_t col,
                     uint16_t sw_value) {
  bool current_state = (*current_row >> col) & 1;
  bool changed = false;
  int key;
  CR2KEY(col, row, key)

#ifdef MINIEC_WEB_TUNING
  const uint8_t press_offset = key_offsets[key].press;
  const uint8_t release_offset = key_offsets[key].release;
#else
  const uint8_t press_offset = HIGH_THRESHOLD;
  const uint8_t release_offset = LOW_THRESHOLD;
#endif

  // press to release
  if (current_state) {
    if (sw_value < calibrate[key].threshold + release_offset) {
      *current_row &= ~(1 << col);
      changed = true;
    }
  } else { // release to press
    if (sw_value > calibrate[key].threshold + press_offset) {
      *current_row |= (1 << col);
      changed = true;
    } else {
      filter_process(&filter[key], (float)sw_value);
    }
  }

  return changed;
}

// Scan key values and update matrix state
bool ecsm_matrix_scan(matrix_row_t current_matrix[]) {
  bool updated = false;
  uint16_t buf;

  for (int col = 0; col < sizeof(col_channels); col++) {
    for (int row = 0; row < sizeof(row_pins); row++) {
      uint16_t val = ecsm_readkey_raw(row, col);
      ecsm_sw_value[row][col] = VAL2THRESHOLD(val);
      updated |= ecsm_update_key(&current_matrix[row], row, col,
                                 ecsm_sw_value[row][col]);
    }
  }

  uint32_t timer_now = timer_read32();
  // Calibration Timer
  if (TIMER_DIFF_32(timer_now, last_timer) > CALIBRATE_FREQ_MS) {
    // Timer Update
    last_timer = timer_now;
    for (uint16_t k = 0; k < KEY_NUM; k++) {
      // The filter already uses ADC/4 units. Round to avoid bias from
      // truncation.
      calibrate[k].buf[calibrate[k].idx] = (uint8_t)(filter[k].output + 0.5f);
      calibrate[k].idx = (calibrate[k].idx + 1) % CALIBRATE_BUF;

      // Average
      buf = 0;
      for (uint16_t i = 0; i < CALIBRATE_BUF; i++) {
        buf += calibrate[k].buf[i];
      }
      calibrate[k].threshold = buf / CALIBRATE_BUF;
    }
  }

  // 1ms Timer
  if (TIMER_DIFF_32(timer_now, timer_1ms) > 1) {
    timer_1ms = timer_now;
  }

  return updated;
}

// Print key values
void ecsm_print_matrix(void) {
  for (int row = 0; row < sizeof(row_pins); row++) {
    for (int col = 0; col < sizeof(col_channels); col++) {
      xprintf("%4d", ecsm_sw_value[row][col]);
    }
    xprintf("\n");
  }
}

static void seed_filter_from_calibration(void) {
  for (uint16_t k = 0; k < KEY_NUM; k++) {
    filter[k].input = calibrate[k].threshold;
    filter[k].output = calibrate[k].threshold;
    filter[k].first = 0;
  }
}

void init_calibrate_info(void) {
  static uint8_t min_buf[KEY_NUM];
  volatile uint32_t tmp = 0;
  uint8_t stable_count = 0;
  uint16_t rounds = 0;
  (void)tmp;

  for (uint16_t k = 0; k < KEY_NUM; k++) {
    min_buf[k] = UINT8_MAX;
  }

  // Ramp-Up
  for (uint16_t k = 0; k < KEY_NUM; k++) {
    tmp = ecsm_readkey_raw_k(k);
    wait_us(30);
  }

  // Wait until all keys stay near their observed minimum for several rounds.
  // This avoids calibrating against transient touches during startup.
  while (stable_count < CALIBRATE_STABLE_COUNT &&
         rounds < CALIBRATE_MAX_ROUNDS) {
    bool stable = true;

    for (uint16_t k = 0; k < KEY_NUM; k++) {
      uint8_t sample = VAL2THRESHOLD(ecsm_readkey_raw_k(k));
      if (sample < min_buf[k]) {
        min_buf[k] = sample;
        stable = false;
      } else if (sample > min_buf[k] + CALIBRATE_STABLE_MARGIN) {
        stable = false;
      }
      wait_us(30);
    }

    stable_count = stable ? (stable_count + 1) : 0;
    rounds++;
  }

  for (uint16_t k = 0; k < KEY_NUM; k++) {
    calibrate[k].threshold = min_buf[k];
    calibrate[k].idx = 0;
    for (uint16_t i = 0; i < CALIBRATE_BUF; i++) {
      calibrate[k].buf[i] = calibrate[k].threshold;
    }
  }
}

void keyboard_post_init_kb() { keyboard_post_init_user(); }

void keyboard_pre_init_kb(void) { keyboard_pre_init_user(); }

void matrix_init_custom(void) {
  // Turn off LED on ProMicro
  DDRD &= ~(1 << 5);
  PORTD &= ~(1 << 5);
  DDRB &= ~(1 << 0);
  PORTB &= ~(1 << 0);

  // initialize matrix state: all keys off
#ifdef MINIEC_WEB_TUNING
  load_key_offsets();
#endif
  ecsm_init();
  init_calibrate_info();

  // Filter config
  for (uint8_t k = 0; k < KEY_NUM; k++) {
    // Low Pass Filter Init
    filter_config(&filter[k], DEFAULT_LOWPASS_HZ, DEFAULT_FILTER_Q,
                  DEFAULT_SAMPLE_HZ);
  }
  seed_filter_from_calibration();
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
  bool updated = ecsm_matrix_scan(current_matrix);

#ifdef CONSOLE_ENABLE
  static int cnt = 0;
  if (cnt++ == 300) {
    print("\033[2J\033[H");
    cnt = 0;
    ecsm_print_matrix();
  }
#endif

  return updated;
}
