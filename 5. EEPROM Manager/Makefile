TARGET = eeprom_manager

C_SOURCES := src\isr.c src\main.c src/print.c src/font_6x8.c src\draw.c
ASM_SOURCES := src\startup.asm

include pm.mk

all: output_name

output_name: $(TARGET).min
	copy /Y "$(TARGET).min" "EEPROM Manager.min"