
/* Copyright 2023 goropikari
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

#include QMK_KEYBOARD_H

#include "quantum.h"

enum layer_names {
    _DVORAK = 0,
    _LOWER,
    _RAISE,
    _ADJUST,
};

// SandS
#define KC_MT_SS MT(MOD_LSFT, KC_SPC)

#define KC_CTLTB CTL_T(KC_TAB)
#define KC_CTL_SCLN CTL_T(KC_SCLN)
#define CTRL_SHFT MT(MOD_LCTL | MOD_LSFT, KC_ESC)
#define XXXXX KC_NO
#define LOWER  MO(_LOWER)
#define RAISE  MO(_RAISE)
#define ADJUST MO(_ADJUST)
#define RAI_ENT LT(_RAISE, KC_ENT)

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_DVORAK] = LAYOUT(
        KC_QUOT     , KC_COMM  , KC_DOT   , KC_P                 , KC_Y    ,                KC_F     , KC_G     , KC_C     , KC_R     , KC_L     ,
        KC_A        , KC_O     , KC_E     , KC_U                 , KC_I    ,                KC_D     , KC_H     , KC_T     , KC_N     , KC_S     ,
        KC_SCLN     , KC_Q     , KC_J     , KC_K                 , KC_X    ,                KC_B     , KC_M     , KC_W     , KC_V     , KC_Z     ,
                          RGUI_T(KC_ESC)  , LT(_LOWER, KC_MINS)  , KC_LCTL ,                RAI_ENT  , KC_MT_SS , KC_RALT
    ),

    [_LOWER] = LAYOUT(
        KC_EXLM     , KC_AT    , KC_HASH  , KC_DLR               , KC_PERC ,               KC_CIRC   , KC_AMPR  , KC_UP    , KC_LPRN  , KC_RPRN  ,
        KC_CTLTB    , KC_A     , KC_Z     , KC_C                 , KC_X    ,               KC_HOME   , KC_LEFT  , KC_DOWN  , KC_RIGHT , KC_BSPC  ,
        XXXXX       , XXXXX    , XXXXX    , XXXXX                , XXXXX   ,               KC_END    , KC_ENT   , KC_LBRC  , KC_RBRC  , KC_DEL   ,
                                 KC_RGUI  , LOWER                , KC_RALT ,               ADJUST    , KC_MT_SS , XXXXX
    ),

    [_RAISE] = LAYOUT(
        KC_1        , KC_2     , KC_3     , KC_4                 , KC_5    ,                KC_LBRC  , KC_RBRC  , KC_TAB   , KC_SLSH  , S(KC_SLSH)  ,
        KC_6        , KC_7     , KC_8     , KC_9                 , KC_0    ,                KC_PIPE  , KC_BSLS  , KC_GRV   , KC_EQL   , KC_RSFT     ,
        KC_LCTL     , KC_LSFT  , XXXXX    , XXXXX                , XXXXX   ,                KC_ENT   , KC_PLUS  , KC_TILD  , XXXXX    , KC_RSFT     ,
                                 KC_RGUI  , ADJUST               , KC_RALT ,                XXXXX    , XXXXX    , XXXXX
    ),

    [_ADJUST] = LAYOUT(
        KC_EXLM     , KC_AT    , KC_HASH  , KC_DLR               , KC_PERC ,              KC_CIRC    , KC_AMPR   , KC_ASTR   , KC_LBRC   , KC_RBRC   ,
        XXXXX       , XXXXX    , XXXXX    , XXXXX                , XXXXX   ,              KC_PIPE    , KC_BSLS   , XXXXX     , S(KC_LBRC), S(KC_RBRC),
        KC_F1       , KC_F2    , KC_F3    , KC_F4                , KC_F5   ,              KC_F6      , KC_F7     , KC_F8     , KC_F9     , KC_F10    ,
                                XXXXX     , XXXXX                , XXXXX   ,                 XXXXX   , XXXXX     , XXXXX
    )
};
