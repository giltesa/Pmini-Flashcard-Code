# PM2040 MultiMenu
A Multi-ROM-Menu for the PM2040 Flash Cart

## Background
This is a menu for loading multiple ROMs on to the [PM2040 flash cart](https://github.com/zwenergy/PM2040) for the Pokemon mini.
With the regular 2 MB Flash of an RP2040-Zero board 2 ROMs can be loaded on the cart.
With custom boards or Flash chip swaps this number can be increased.

## Usage
The menu is **not** auto-patching.
This is critical for most commercial games, since these games do not reset the Pokemon mini when powering off, but save the current state which is restored after powering on again.
This behavior is problematic for the multi-menu, as the PM2040 flash cart does not save the last loaded ROM, hence the multi-menu starts but the internal state of the Pokemon mini corresponds to the last loaded game.
Hence, this will easily lead to hard crashes.

To avoid this, apply the corresponding patches to the ROMs or briefly remove the flash cart after powering off, as this will reset the internal state of the Pokemon mini.

Also, the menu is **not** auto-parsing games.
The menu needs to be compiled with the number of slots required.
Pre-compiled binaries are available in the release-section.

To create a multi-ROM UF2 firmware file for the PM2040, a compiled menu binary needs to be converted into an array file (see the PM2040 repo) and inclued into the PM2040 source.
See the PM2040 source for more details.

## Building
This project uses the [Epson S1C88 C Tools for Pokemon Mini](https://github.com/pokemon-mini/c88-pokemini) to be built.

## Resources
This project took a lot of inspiration of zoranc's ROM menu for the Ditto mini.
And also from the S1C88 C Tools for Pokemon Mini-project.

Many many thanks go to them, as they are the main foundation for this!
