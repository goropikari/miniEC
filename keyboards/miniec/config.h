/*
Copyright 2020 sekigon-gonnoc
Copyright 2023 goropikari (@goropikari)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

// #define SELECT_SOFT_SERIAL_SPEED 3
// #define SERIAL_DEBUG
/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
//#define NO_DEBUG

/* disable print */
//#define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT
/* key matrix size */
#define MATRIX_ROWS 6
#define MATRIX_COLS 6

/* EC switch threshold with hysteresis */
// for v1.1.1
#define HIGH_THRESHOLD 130
#define LOW_THRESHOLD 120

#define TAP_CODE_DELAY 5
#define TAPPING_TERM 125
#define PERMISSIVE_HOLD

/*
 * Keyboard Matrix Assignments
 */
#define MATRIX_ROW_PINS { D4, C6, D7 }
#define COL_PINS { S1, S0, S3, S5, S7, S2 }
#define DISCHARGE_PIN B1
#define ANALOG_PORT F6
#define MUX_SEL_PINS { B6, B2, B3 }


/*
 * Split Keyboard specific options, make sure you have 'SPLIT_KEYBOARD = yes' in your rules.mk, and define SOFT_SERIAL_PIN.
 */
#define SOFT_SERIAL_PIN D2  // or D1, D2, D3, E6
#define SPLIT_HAND_PIN F4
// #define EE_HANDS
// #define MASTER_LEFT


/* Debounce reduces chatter (unintended double-presses) - set 0 if debouncing is not needed */
#define DEBOUNCE 5

/* define if matrix has ghost (lacks anti-ghosting diodes) */
//#define MATRIX_HAS_GHOST

/* Mechanical locking support. Use KC_LCAP, KC_LNUM or KC_LSCR instead in keymap */
#define LOCKING_SUPPORT_ENABLE
/* Locking resynchronize hack */
#define LOCKING_RESYNC_ENABLE

/* If defined, GRAVE_ESC will always act as ESC when CTRL is held.
 * This is useful for the Windows task manager shortcut (ctrl+shift+esc).
 */
//#define GRAVE_ESC_CTRL_OVERRIDE
