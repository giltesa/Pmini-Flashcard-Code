TARGET = eeprom_manager

C_SOURCES := src\isr.c src\main.c src/print.c src/font_6x8.c src\draw.c src\save.c src\font_5x5.c src\print_small.c
ASM_SOURCES := src\startup.asm

include pm.mk

all: output_name

output_name: $(TARGET).min
#	copy /Y "$(TARGET).min" "EEPROM Manager.min"
	cmd /c move /Y "$(TARGET).min" "EEPROM Manager.min"