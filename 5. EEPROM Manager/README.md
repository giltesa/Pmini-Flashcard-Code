# PM2040 EEPROM Manager

This directory contains a fork of the original [PM2040 EEPROM Manager](https://github.com/zwenergy/PM2040-EEPROM-Manager) created by [zwenergy](https://github.com/zwenergy).

The original project provides EEPROM backup and restore functionality for PM2040-based Pokémon Mini flash carts. This fork keeps that functionality while extending the interface and adding save inspection and management features for the Pokémon Mini EEPROM.

## Features

- Backup the physical Pokémon Mini EEPROM to one of three PM2040 backup slots.
- Restore any of the three backup slots to the physical EEPROM.
- View the conventional Pokémon Mini save directory from:
  - the physical EEPROM;
  - `SLOT 1`;
  - `SLOT 2`;
  - `SLOT 3`.
- Display up to six save entries using the compact 5x5 font.
- Show `EMPTY`, `INVALID` or `UNKNOWN` when an entry cannot be displayed as a valid save.
- Delete individual saves from the **physical EEPROM**.
  - Deletion clears only the selected 18-byte save header.
  - Save payload data is left untouched.
  - The write is split at EEPROM page boundaries when required.
  - The written header is read back and verified before returning to the list.
- Backup slots are intentionally read-only from the VIEW screen. Individual saves cannot be deleted from `SLOT 1`, `SLOT 2` or `SLOT 3`.

## PM2040 mapping

This version expects the active EEPROM / selected backup image to be exposed at:

```c
#define CARTEEPROM 0x6000
```

The corresponding PM2040 firmware must use the matching EEPROM offset:

```c
#define EEPROM_OFFSET 0x6000
```

This leaves `0x0000..0x5FFF` available for the EEPROM Manager and reserves `0x6000..0x7FFF` for the active 8 KiB EEPROM image.

## Notes

- Individual DELETE is intentionally limited to the physical EEPROM.
- Backup slots remain writable through the existing full `BACKUP` workflow and readable/restorable through the normal Manager interface.
- Physical EEPROM writes use the existing low-level EEPROM timing model and fixed write-completion delay; the driver does not implement ACK polling.

## Credits

Original EEPROM Manager and PM2040 firmware by [zwenergy](https://github.com/zwenergy).

UI redesign, VIEW/save management features and PM2040 integration changes by [Giltesa](https://github.com/giltesa).
