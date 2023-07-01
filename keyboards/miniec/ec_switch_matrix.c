/* Copyright 2012 Jun Wako <wakojun@gmail.com>
 * Copyright 2020 sekigon-gonnoc
 * Copyright 2020 ginjake
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

#include "ec_switch_matrix.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "print.h"
#include "debug.h"
#include "util.h"
#include "matrix.h"
#include "split_util.h"
#include "quantum.h"

#include "analog.h"
// #include "print.h"
// #include "quantum.h"

// sensing channel definitions
#define S0 0
#define S1 1
#define S2 2
#define S3 3
#define S4 4
#define S5 5
#define S6 6
#define S7 7

#define ROWS_PER_HAND (MATRIX_ROWS/2)
#define KEY_NUM (MATRIX_COLS*ROWS_PER_HAND)
#define KEY2CR(K,C,R) {(C)=(K)%MATRIX_COLS;(R)=(K)/MATRIX_COLS;}
#define VAL2THRESHOLD(VAL) (((VAL)>=1024) ? 255 : (VAL/4))
#define PI ((float)3.1415926535)
#define CALIBRATE_BUF 5
#define CALIBRATE_INI 20
#define CALIBRATE_FREQ_MS 10000
#define DEFAULT_THRESHOLD_DOWN 80
#define DEFAULT_THRESHOLD_UP 20
#define DEFAULT_FILTER_Q (1.0/sqrt(2.0))
#define DEFAULT_SAMPLE_HZ 150.0
#define DEFAULT_LOWPASS_HZ 1.0
// #define _barrier() asm volatile ("" ::: "memory")
// static const uint8_t row_pins[] = MATRIX_ROW_PINS;
static const uint8_t col_pins[] = MUX_SEL_PINS;

typedef struct{
    uint8_t threshold;
    uint8_t buf[CALIBRATE_BUF];
    uint8_t idx;
} calibrate_info;

uint8_t key_val[KEY_NUM];

calibrate_info calibrate[KEY_NUM];
uint32_t last_timer;

/* Filter */
typedef struct{
    float a; // Feed forward coefficient
    float b; // Feedback coefficient

    float input;
    float output;
    uint8_t first;
} filter_info;

filter_info filter[KEY_NUM];


// calibration
void finter_config(filter_info *f, float fc, float q, float fs){

    //fc Digital LPF cutoff frequency
    //q  Quality factor (1 / sqrt (2) default)
    //fs Sampling frequency

    float t = 1.0/fs;
    float c = 1.0*PI*fc;
    f->b = tan(c*t/2.0)/(1.0+tan(c*t/2.0));
    f->a = (1.0-tan(c*t/2.0))/(1.0+tan(c*t/2.0));
    f->first = 1;
}

float filter_process(filter_info *f, float val){
    float result;
    if(f->first){
        f->input = val;
        f->output = val;
        f->first = 0;
    }

    result = f->b*val + f->b*f->input + f->a*f->output;
    f->input = val;
    f->output = result;
    return result;
}

/* Spin WAIT */
void spin_wait(volatile uint32_t time){
    while(time>0){
        time--;
    }
}

void init_calibrate_info(void){
    static uint16_t buf[KEY_NUM] = {0};
    volatile uint32_t tmp = 0;
    (void)tmp;

    // Ramp-Up
    for(uint16_t k=0;k<KEY_NUM;k++){
        uint8_t row, col;
        KEY2CR(k, row, col);
        tmp = ecsm_readkey_raw(row, col);
        // SPIN WAIT(While DISCHARGE COMPLETE)
        spin_wait(10);
    }

    // Calibration
    for(uint16_t i=0;i<CALIBRATE_INI;i++){
        for(uint16_t k=0;k<KEY_NUM;k++){
            uint8_t row, col;
            KEY2CR(k, row, col);
            buf[k] += ecsm_readkey_raw(row, col);
            // SPIN WAIT(While DISCHARGE COMPLETE)
            spin_wait(10);
        }
    }

    for(uint16_t k=0;k<KEY_NUM;k++){
        // Initialize
        calibrate[k].threshold = VAL2THRESHOLD(buf[k]/CALIBRATE_INI);
        calibrate[k].idx = 0;
        // Average
        for(uint16_t i=0;i<CALIBRATE_BUF;i++){
            calibrate[k].buf[i] = calibrate[k].threshold;
        }
    }
}

// pin connections
const uint8_t row_pins[]     = MATRIX_ROW_PINS;
const uint8_t col_channels[] = COL_PINS;
const uint8_t mux_sel_pins[] = MUX_SEL_PINS;

_Static_assert(sizeof(mux_sel_pins) == 3, "invalid MUX_SEL_PINS");

static ecsm_config_t config;
static uint16_t      ecsm_sw_value[MATRIX_ROWS][MATRIX_COLS];

static inline void discharge_capacitor(void) {
    setPinOutput(DISCHARGE_PIN);
}

static inline void charge_capacitor(uint8_t row) {
    setPinInput(DISCHARGE_PIN);
    writePinHigh(row_pins[row]);
}

static inline void clear_all_row_pins(void) {
    for (int row = 0; row < sizeof(row_pins); row++) {
        writePinLow(row_pins[row]);
    }
}

static inline void init_mux_sel(void) {
    for (int idx = 0; idx < sizeof(mux_sel_pins); idx++) {
        setPinOutput(mux_sel_pins[idx]);
    }
}

static inline void select_mux(uint8_t col) {
    uint8_t ch = col_channels[col];
    writePin(mux_sel_pins[0], ch & 1);
    writePin(mux_sel_pins[1], ch & 2);
    writePin(mux_sel_pins[2], ch & 4);
}

static inline void init_row(void) {
    for (int idx = 0; idx < sizeof(row_pins); idx++) {
        setPinOutput(row_pins[idx]);
        writePinLow(row_pins[idx]);
    }
}

// Initialize pins
int ecsm_init(ecsm_config_t const* const ecsm_config) {
    // save config
    config = *ecsm_config;

    // initialize discharge pin as discharge mode
    writePinLow(DISCHARGE_PIN);
    setPinOutput(DISCHARGE_PIN);

    // set analog reference
    analogReference(ADC_REF_POWER);

    // initialize drive lines
    init_row();

    // initialize multiplexer select pin
    init_mux_sel();

    // set discharge pin to charge mode
    setPinInput(DISCHARGE_PIN);

    init_calibrate_info();

    return 0;
}

int scan_cnt = 0;
long long scan_avg = 0;

// Read key value of key (row, col)
uint16_t ecsm_readkey_raw(uint8_t row, uint8_t col) {
    uint16_t sw_value = 0;

    discharge_capacitor();

    select_mux(col);

    clear_all_row_pins();

    cli();

    charge_capacitor(row);

    sw_value = analogReadPin(ANALOG_PORT);

    sei();

    return VAL2THRESHOLD(sw_value);
}

// Update press/release state of key at (row, col)
bool ecsm_update_key(matrix_row_t* current_row, uint8_t row, uint8_t col, uint16_t sw_value) {
    bool current_state = (*current_row >> col) & 1;

    // press to release
    if (current_state && sw_value < config.low_threshold[row][col]) {
        *current_row &= ~(1 << col);
        return true;
    }

    // release to press
    uint8_t k = row * (MATRIX_ROWS / 2) + col;
    if ((!current_state)) {
        if (sw_value > calibrate[k].threshold + config.high_threshold[row][col]) {
            *current_row |= (1 << col);
            return true;
        } else {
            filter_process(&filter[k], (float)sw_value);
        }
    }

    return false;
}

// Scan key values and update matrix state
bool ecsm_matrix_scan(matrix_row_t current_matrix[]) {
    bool updated = false;

    for (int col = 0; col < sizeof(col_channels); col++) {
        for (int row = 0; row < sizeof(row_pins); row++) {
            ecsm_sw_value[row][col] = ecsm_readkey_raw(row, col);
            updated |= ecsm_update_key(&current_matrix[row], row, col, ecsm_sw_value[row][col]);
        }
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
