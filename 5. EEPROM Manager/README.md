
# PM2040 EEPROM Manager

This directory contains a fork of the original [PM2040 EEPROM Manager](https://github.com/zwenergy/PM2040-EEPROM-Manager) created by [zwenergy](https://github.com/zwenergy).

The original project provides EEPROM backup and restore functionality for PM2040-based Pokémon Mini flash carts.

## Changes in this fork

This fork keeps the original EEPROM backup and restore functionality while redesigning the user interface to match the MultiROM menu used by this flash cart.

Changes include:

- Redesigned graphical interface.
- UI styling consistent with the MultiROM menu.
- Improved menu navigation and screen layout.
- Backup and restore slots displayed as `SLOT 1`, `SLOT 2` and `SLOT 3`.
- Context-sensitive button hints:
  - `A:NEXT` in the main menu.
  - `B:BACK` / `A:START` when selecting a slot.
  - `B:BACK` after a backup or restore operation has completed.
- Completion screens can only be dismissed with the `B` button.
- Updated 6x8 font and drawing routines shared with the MultiROM menu.
- Precompiled `eeprom_manager.min` included for use with the supported   MultiROM firmware.

## Credits

Original EEPROM Manager and PM2040 firmware by [zwenergy](https://github.com/zwenergy).
UI redesign and integration changes by [Giltesa](https://github.com/giltesa).

## Usage

Usage instructions are included in the main flash cart documentation.
