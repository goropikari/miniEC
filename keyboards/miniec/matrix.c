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

#include "quantum.h"
#include "ec_switch_matrix.h"
#include "matrix.h"
#include "debug.h"
#include "split_util.h"
#include "transport.h"
#include "debounce.h"

#define ERROR_DISCONNECT_COUNT 20

#define ROWS_PER_HAND (MATRIX_ROWS / 2)

/* matrix state(1:on, 0:off) */
extern matrix_row_t raw_matrix[MATRIX_ROWS];  // raw values
extern matrix_row_t matrix[MATRIX_ROWS];      // debounced values

// row offsets for each hand
uint8_t thisHand, thatHand;

// user-defined overridable functions
__attribute__((weak)) void matrix_slave_scan_user(void) {}

void matrix_init_custom(void) {
    // Turn off LED on ProMicro
    DDRD &= ~(1<<5);
    PORTD &= ~(1<<5);
    DDRB &= ~(1<<0);
    PORTB &= ~(1<<0);

    split_pre_init();

    ecsm_config_t ecsm_config;
    for (int r = 0; r < MATRIX_ROWS / 2; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            ecsm_config.low_threshold[r][c] = LOW_THRESHOLD;
            ecsm_config.high_threshold[r][c] = HIGH_THRESHOLD;
        }
    }
    if (isLeftHand) { // left side hand
        // specific configuration
    } else { // right side hand
        // キー毎に閾値を調整する例
        // uint16_t lows[MATRIX_ROWS / 2][MATRIX_COLS] = {
        //     { 0, 11, 10,  0,  0, 25},
        //     { 0, 15, 20, 15,  0,  0},
        //     { 0, 15, 25,  0,  0,  0},
        // };
        // uint16_t highs[MATRIX_ROWS / 2][MATRIX_COLS] = {
        //     { 0, 13, 15,  0,  0, 30},
        //     { 0, 20, 22, 20,  0,  0},
        //     { 0, 20, 30,  0,  0,  0},
        // };
        // for (int r = 0; r < MATRIX_ROWS / 2; r++) {
        //     for (int c = 0; c < MATRIX_COLS; c++) {
        //         if (lows[r][c] != 0) ecsm_config.low_threshold[r][c] = lows[r][c];
        //         if (highs[r][c] != 0) ecsm_config.high_threshold[r][c] = highs[r][c];
        //     }
        // }
    }

    ecsm_init(&ecsm_config);

    thisHand = isLeftHand ? 0 : (ROWS_PER_HAND);
    thatHand = ROWS_PER_HAND - thisHand;

    split_post_init();
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool updated = ecsm_matrix_scan(current_matrix);

#ifdef CONSOLE_ENABLE
    static int cnt = 0;
    if (cnt++ == 300) {
        cnt = 0;
        ecsm_print_matrix();
        print("\n");
    }
#endif

    return updated;
}

uint8_t matrix_scan(void) {
    bool changed = matrix_scan_custom(raw_matrix);

    debounce(raw_matrix, matrix + thisHand, ROWS_PER_HAND, changed);

    matrix_post_scan();
    return changed;
}
