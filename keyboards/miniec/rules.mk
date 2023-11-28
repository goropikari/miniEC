# This file intentionally left blank
# MCU name
MCU = atmega32u4

# Bootloader selection
BOOTLOADER = caterina

# Do not enable SLEEP_LED_ENABLE. it uses the same timer as BACKLIGHT_ENABLE
SLEEP_LED_ENABLE = no       # Breathing sleep LED during USB suspend
# if this doesn't work, see here: https://github.com/tmk/tmk_keyboard/wiki/FAQ#nkro-doesnt-work
BACKLIGHT_ENABLE = no       # Enable keyboard backlight functionality
RGBLIGHT_ENABLE = no        # Enable keyboard RGB underglow
BLUETOOTH_ENABLE = no       # Enable Bluetooth
AUDIO_ENABLE = no           # Audio output
CUSTOM_MATRIX = lite
SPLIT_KEYBOARD = yes
# DEBOUNCE_TYPE = sym_eager_pk
ANALOG_DRIVER_REQUIRED = yes

SRC += ec_switch_matrix.c matrix.c
