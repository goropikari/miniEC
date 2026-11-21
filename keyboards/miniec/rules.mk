# MCU name
MCU = atmega32u4

# Bootloader selection
BOOTLOADER = caterina

CUSTOM_MATRIX = lite
SPLIT_KEYBOARD = yes
# SERIAL_DRIVER = bitbang
# DEBOUNCE_TYPE = sym_eager_pk
ANALOG_DRIVER_REQUIRED = yes

SRC += matrix.c

OPT_DEFS += -DMINIEC_WEB_TUNING
LTO_ENABLE = yes
