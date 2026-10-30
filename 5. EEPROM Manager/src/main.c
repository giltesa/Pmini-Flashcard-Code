//-----------------------------------------------------------------------
//-- Title: EEPROM manager for PM2040 flash cart
//-- Author: zwenergy
//-----------------------------------------------------------------------

#include "pm.h"
#include "print.h"
#include "draw.h"
#include "save.h"
#include "print_small.h"

#include <stdint.h>
#include <string.h>



#define DELAY 100
#define DELAYMAX 255

#define EEPROMSLOTS 3

// Manual debounce
#define DEBOUNCELEN 20

// Addresses.
// Set EEPROM slot. 0b0100001111000001
#define REG_SLOT 0x43C1

// Set lower address. 0b0100001111000010
#define REG_LOADDR 0x43C2

// Set upper address. 0b0100001111000011
#define REG_HIADDR 0x43C3

// Write byte. 0b0100001111000100
#define REG_BYTE 0x43C4

// Flush temp buffer to RP2040 Flash. 0b0100001111000111
#define REG_STOREFLASH 0x43C7

#define MAXEEPROM 8191

// RP2040 EEPROM temp address.
#define CARTEEPROM 0x6000


volatile uint8_t flag;

const uint8_t readCmd = 0xA1;
const uint8_t writeCmd = 0xA0;

enum menu {
  MAIN,
  BACKUP,
  RESTORE,
  VIEW,
  VIEW_RESULT,
  DELETE_CONFIRM
};

void delay( void ) {
  volatile cnt, dummy;

  // Very pretty delay function.
  for ( cnt = 0; cnt < DELAY; ++cnt ) {
    dummy++;
  }
}


void delayLong( void ) {
  volatile cnt, dummy;

  // Very pretty delay function.
  for ( cnt = 0; cnt < DELAYMAX; ++cnt ) {
    dummy++;
  }

  for ( cnt = 0; cnt < DELAYMAX; ++cnt ) {
    dummy++;
  }

  for ( cnt = 0; cnt < DELAYMAX; ++cnt ) {
    dummy++;
  }

  for ( cnt = 0; cnt < DELAYMAX; ++cnt ) {
    dummy++;
  }

  for ( cnt = 0; cnt < DELAYMAX; ++cnt ) {
    dummy++;
  }

  for ( cnt = 0; cnt < DELAYMAX; ++cnt ) {
    dummy++;
  }
}

void EEPROM_SCL_LO ( void ) {
  IO_DATA = ( IO_DATA & 0xF7 );
}

void EEPROM_SCL_HI ( void ) {
  IO_DATA = ( IO_DATA | 0x08 );
}

void EEPROM_SDA_HI ( void ) {
  IO_DATA = ( IO_DATA | 0x04 );
}

void EEPROM_SDA_LO ( void ) {
  IO_DATA = ( IO_DATA & 0xFB );
}



void writeDir( void ) {
  IO_DIR = IO_DIR | 0x04;
}

void readDir( void ) {
  IO_DIR = IO_DIR & 0xFB;
}

void startCondition( void ) {
  // Start condition
  EEPROM_SDA_HI();
  EEPROM_SCL_HI();

  delay();

  EEPROM_SDA_LO();

  delay();

  EEPROM_SCL_LO();
}

void sendBusBit( uint8_t b ) {
  writeDir();
  // We assume the clock is high.

  // Set clock low.
  EEPROM_SCL_LO();

  // Set the bit.
  if ( b ) {
    EEPROM_SDA_HI();
  } else {
    EEPROM_SDA_LO();
  }

  // Set the clock high.
  EEPROM_SCL_HI();
}


void sendByte( uint8_t b ) {
  int8_t i;

  writeDir();
  for( i = 7; i >= 0; --i ) {
    sendBusBit( b & ( 1 << i ) );
  }

  // Ack.
  EEPROM_SCL_LO();
  readDir();
  EEPROM_SCL_HI();
}


void readEEPROMBytes( uint16_t a, uint8_t* buffer, uint16_t bytes ) {
  uint8_t c, i;
  uint16_t byteCnt;

  // Set bus direction.
  IO_DIR = ( IO_DIR | 0x04 | 0x08 );

  // Set start condition.
  startCondition();

  // Send write command.
  sendByte( writeCmd );

  // Send upper address.
  sendByte( a >> 8 );

  // Send lower address.
  sendByte( a );

  EEPROM_SCL_LO();
  writeDir();
  EEPROM_SDA_HI();

  EEPROM_SCL_HI();

  // Start condition.
  startCondition();

  // Send read command.
  sendByte( readCmd );

  // Read data.
  for ( byteCnt = 0; byteCnt < bytes; ++byteCnt ) {
    c = 0;
    for ( i = 0; i < 8; ++i ) {
      EEPROM_SCL_LO();
      EEPROM_SCL_HI();
      c = ( c | ( ( ( IO_DATA & 0x04 ) >> 2 ) << ( 7 - i ) ) );
    }

    // Store byte.
    buffer[ byteCnt ] = c;

    if ( byteCnt != ( bytes - 1 ) ) {
      // Set ack.
      EEPROM_SCL_LO();
      writeDir();
      EEPROM_SDA_LO();
      EEPROM_SCL_HI();
      EEPROM_SCL_LO();
      readDir();
    }
  }

  // No ack.
  EEPROM_SCL_LO();
  writeDir();
  EEPROM_SDA_LO();
  EEPROM_SCL_HI();

  // Stop condition.
  EEPROM_SDA_HI();
}


void copyToEEPROM() {
  uint8_t page, i, c;
  uint16_t addr;
  c = 0;
  i = 0;
  page = 0;
  addr = 0;

  // Go over page-wise.
  while( 1 ) {
    // Set bus direction.
    IO_DIR = ( IO_DIR | 0x04 | 0x08 );

    startCondition();

    // Send write command.
    sendByte( writeCmd );

    // Send upper address.
    sendByte( addr >> 8 );

    // Send lower address.
    sendByte( addr );

    // Send 32 bytes.
    for ( i = 0; i < 32; ++i ) {
      // Read byte from RP.
      c =  *( (uint8_t *) ( CARTEEPROM + addr ) );
      ++addr;

      sendByte( c );
    }

    EEPROM_SCL_LO();
    writeDir();
    EEPROM_SDA_LO();

    EEPROM_SCL_HI();

    delay();

    // Stop condition.
    EEPROM_SDA_HI();

    // Wait for write. This takes up to 5 ms.
    delayLong();

    if ( page == 255 ) {
      break;
    }
    ++page;
  }
}


// Bounded physical EEPROM writes; preserve RESTORE's page/STOP/delay sequence.
static void writeEEPROMRange( uint16_t addr, const uint8_t* data, uint16_t bytes ) {
  uint8_t chunk, i;

  if ( addr > MAXEEPROM || bytes > MAXEEPROM + 1 - addr ) {
    return;
  }

  while ( bytes ) {
    chunk = 32 - ( addr % 32 );
    if ( bytes < chunk ) {
      chunk = bytes;
    }

    IO_DIR = ( IO_DIR | 0x04 | 0x08 );
    startCondition();
    sendByte( writeCmd );
    sendByte( addr >> 8 );
    sendByte( addr );
    for ( i = 0; i < chunk; ++i ) {
      sendByte( *data );
      ++data;
    }

    EEPROM_SCL_LO();
    writeDir();
    EEPROM_SDA_LO();
    EEPROM_SCL_HI();
    delay();
    EEPROM_SDA_HI();

    // Same fixed programming wait as RESTORE, including the final chunk.
    // The existing driver does not sample ACK or perform ACK polling.
    delayLong();
    addr += chunk;
    bytes -= chunk;
  }
}


uint8_t keyScan( void ) {
  uint8_t i;
  uint8_t k = KEY_PAD;

  // Debounce.
  for ( i = 0; i < DEBOUNCELEN; ++i ) {
    if ( k == KEY_PAD ) {
      continue;
    } else {
      return -1;
    }
  }

  return k;
}

_interrupt( 2 ) void prc_frame_copy_irq(void)
{
  flag = 1;
  IRQ_ACT1 = IRQ1_PRC_COMPLETE;
}

enum menu curMenu = MAIN;

// -----------------------------------------------------------------------------
// UI layout
// -----------------------------------------------------------------------------
#define UI_TAB_Y        0
#define UI_TAB_H        11
#define UI_TAB_W        68
#define UI_TAB_BEVEL    4

#define UI_CONTENT_X    0
#define UI_CONTENT_Y    ( UI_TAB_Y + UI_TAB_H - 1 )
#define UI_CONTENT_W    LCDWIDTH
#define UI_CONTENT_H    ( LCDHEIGHT - UI_CONTENT_Y )

#define UI_CURSOR_X     3
#define UI_LABEL_X      12
#define UI_MAIN_Y       15
#define UI_SLOT_Y       15
#define UI_ITEM_STEP    12
#define UI_VIEW_Y       14
#define UI_VIEW_STEP    10


static void clearScreen( void ) {
  memset( (void*) FRAMEBUFF, 0, LCDWIDTH * ( LCDHEIGHT / 8 ) );
}


static void drawFrameWithTab( const char* title ) {
  clearScreen();

  // Same visual language as the MultiROM menu: black active tab,
  // white text and a framed content area.
  drawActiveTab( UI_TAB_Y, UI_TAB_Y, UI_TAB_W, UI_TAB_H, UI_TAB_BEVEL );
  printPx( 2, UI_TAB_Y + 2, title, WHITE );

  drawRect( UI_CONTENT_X, UI_CONTENT_Y, UI_CONTENT_W, UI_CONTENT_H, BLACK );
}


static int itemY( uint8_t index ) {
  if ( curMenu == VIEW ) {
    return UI_VIEW_Y + ( index * UI_VIEW_STEP );
  }

  if ( curMenu == MAIN ) {
    return UI_MAIN_Y + ( index * UI_ITEM_STEP );
  }

  return UI_SLOT_Y + ( index * UI_ITEM_STEP );
}


static void drawCursor( uint8_t index ) {
  printCharPx( UI_CURSOR_X, itemY( index ), '>', BLACK_ON_WHITE );
}


static void clearCursor( uint8_t index ) {
  printCharPx( UI_CURSOR_X, itemY( index ), ' ', BLACK_ON_WHITE );
}


static void drawSlotLabel( uint8_t index ) {
  int y = UI_SLOT_Y + ( index * UI_ITEM_STEP );

  printPx( UI_LABEL_X, y, "SLOT", BLACK );
  // Internal slots are 0..2, but the UI matches EEPROM01..EEPROM03.
  printDigitPx( UI_LABEL_X + 30, y, (unsigned char)( index + 1 ), BLACK );
}


void drawMenu( void ) {
  uint8_t i;

  if ( curMenu == MAIN ) {
    drawFrameWithTab( "EEPROM MGR" );
    printPx( UI_LABEL_X, UI_MAIN_Y, "VIEW", BLACK );
    printPx( UI_LABEL_X, UI_MAIN_Y + UI_ITEM_STEP, "BACKUP", BLACK );
    printPx( UI_LABEL_X, UI_MAIN_Y + 2 * UI_ITEM_STEP, "RESTORE", BLACK );

    printSmallNavPx( LCDWIDTH - 3 - smallNavWidth( "A:NEXT" ), 56, "A:NEXT", BLACK );

  } else if ( curMenu == VIEW ) {
    drawFrameWithTab( "VIEW" );
    printPx( UI_LABEL_X, UI_VIEW_Y, "EEPROM", BLACK );

    for ( i = 1; i <= EEPROMSLOTS; ++i ) {
      printPx( UI_LABEL_X, UI_VIEW_Y + i * UI_VIEW_STEP, "SLOT", BLACK );
      printDigitPx( UI_LABEL_X + 30, UI_VIEW_Y + i * UI_VIEW_STEP, i, BLACK );
    }

    printSmallNavPx( 3, 56, "B:BACK", BLACK );
    printSmallNavPx( LCDWIDTH - 3 - smallNavWidth( "A:VIEW" ), 56, "A:VIEW", BLACK );

  } else if ( curMenu == BACKUP ) {
    drawFrameWithTab( "BACKUP" );

    for ( i = 0; i < EEPROMSLOTS; ++i ) {
      drawSlotLabel( i );
    }

    printSmallNavPx( 3, 56, "B:BACK", BLACK );
    printSmallNavPx( LCDWIDTH - 3 - smallNavWidth( "A:START" ), 56, "A:START", BLACK );

  } else if ( curMenu == RESTORE ) {
    drawFrameWithTab( "RESTORE" );

    for ( i = 0; i < EEPROMSLOTS; ++i ) {
      drawSlotLabel( i );
    }

    printSmallNavPx( 3, 56, "B:BACK", BLACK );
    printSmallNavPx( LCDWIDTH - 3 - smallNavWidth( "A:START" ), 56, "A:START", BLACK );
  }
}


static void drawOperationScreen( enum menu operation, uint8_t slot, uint8_t done ) {
  if ( operation == BACKUP ) {
    drawFrameWithTab( "BACKUP" );

    if ( done ) {
      printPx( 15, 23, "BACKUP DONE", BLACK );
    } else {
      printPx( 21, 23, "BACKUP...", BLACK );
    }

  } else {
    drawFrameWithTab( "RESTORE" );

    if ( done ) {
      printPx( 12, 23, "RESTORE DONE", BLACK );
    } else {
      printPx( 15, 23, "RESTORING...", BLACK );
    }
  }

  printPx( 27, 36, "SLOT", BLACK );
  printDigitPx( 57, 36, (unsigned char)( slot + 1 ), BLACK );

  if ( done ) {
    printSmallNavPx( 3, 56, "B:BACK", BLACK );
  }
}


void waitForButton( void ) {
  // Wait for all keys to be released first.
  while ( keyScan() != 0xFF ) {
  }

  // Then wait specifically for B.
  while ( keyScan() & KEY_B ) {
  }
}


uint8_t readBuffer[ 32 ];


static uint8_t deleteEEPROMHeader( uint8_t source, uint8_t index ) {
  uint16_t addr;
  uint8_t i;

  if ( source != 0 || index >= SAVECOUNT ) {
    return 0;
  }

  addr = 4 + index * SAVEHEADERSIZE;
  readEEPROMBytes( addr, readBuffer, SAVEHEADERSIZE );
  if ( classifySaveHeader( readBuffer ) != SAVE_OCCUPIED ) {
    return 0;
  }

  memset( readBuffer, 0, SAVEHEADERSIZE );
  writeEEPROMRange( addr, readBuffer, SAVEHEADERSIZE );
  readEEPROMBytes( addr, readBuffer, SAVEHEADERSIZE );
  for ( i = 0; i < SAVEHEADERSIZE; ++i ) {
    if ( readBuffer[ i ] != 0 ) {
      return 0;
    }
  }
  return 1;
}


static void readViewBytes( uint8_t source, uint16_t addr, uint8_t bytes ) {
  uint8_t i;

  // All callers use the fixed conventional directory, at offsets 0..111.
  if ( !source ) {
    readEEPROMBytes( addr, readBuffer, bytes );
  } else {
    for ( i = 0; i < bytes; ++i ) {
      readBuffer[ i ] = *( (volatile uint8_t *) ( CARTEEPROM + addr + i ) );
    }
  }
}


static void drawViewCursor( uint8_t index, int color ) {
  int y = 14 + index * 6;

  printSmallPx( 4, y, ">", color );
}


static void drawViewScreen( uint8_t source ) {
  uint8_t i, kind, magic;
  uint16_t addr = 4;
  const char* title;

  if ( !source ) {
    drawFrameWithTab( "EEPROM" );
  } else {
    drawFrameWithTab( "SLOT" );
    printDigitPx( 32, UI_TAB_Y + 2, source, WHITE );

    // Selection only: VIEW never transfers bytes or stores backup flash.
    *( (volatile uint8_t *) REG_SLOT ) = source - 1;
  }

  readViewBytes( source, 0, 4 );
  magic = ( readBuffer[ 0 ] == 'G' && readBuffer[ 1 ] == 'B' &&
            readBuffer[ 2 ] == 'M' && readBuffer[ 3 ] == 'N' );

  for ( i = 0; i < SAVECOUNT; ++i ) {
    readViewBytes( source, addr, SAVEHEADERSIZE );
    kind = classifySaveHeader( readBuffer );

    if ( kind == SAVE_OCCUPIED ) {
      title = saveTitle( readBuffer );
    } else if ( kind == SAVE_FREE ) {
      title = magic ? "EMPTY" : "UNKNOWN";
    } else {
      title = "INVALID";
    }

    // Five-pixel names with one blank row between entries.
    printSmallPx( 10, 14 + i * 6, title, BLACK );
    addr += SAVEHEADERSIZE;
  }

  printSmallNavPx( 3, 56, "B:BACK", BLACK );
  if ( !source ) {
    printSmallNavPx( LCDWIDTH - 3 - smallNavWidth( "A:DELETE" ), 56, "A:DELETE", BLACK );
  }
}


static void drawDeleteScreen( uint8_t source, uint8_t index ) {
  const char* title;
  const char* prompt = "DELETE SAVE?";
  int lineHeight = 8;
  int lineGap = 4;
  int footerY = 54;
  int bodyTop = UI_CONTENT_Y + 1;
  int bodyY = bodyTop + ( footerY - bodyTop - ( 2 * lineHeight + lineGap ) ) / 2;
  int titleWidth, promptWidth;

  if ( !source ) {
    drawFrameWithTab( "EEPROM" );
  } else {
    drawFrameWithTab( "SLOT" );
    printDigitPx( 32, UI_TAB_Y + 2, source, WHITE );
  }

  // Read only the selected header; no stored title or header mirror.
  readViewBytes( source, 4 + index * SAVEHEADERSIZE, SAVEHEADERSIZE );
  if ( classifySaveHeader( readBuffer ) == SAVE_OCCUPIED ) {
    title = saveTitle( readBuffer );
  } else {
    title = "INVALID";
  }

  titleWidth = strlen( title ) * CHARWIDTH;
  promptWidth = strlen( prompt ) * CHARWIDTH;
  printPx( ( LCDWIDTH - titleWidth ) / 2, bodyY, title, BLACK );

  printPx( ( LCDWIDTH - promptWidth ) / 2, bodyY + lineHeight + lineGap, prompt, BLACK );
  printSmallNavPx( 3, 56, "B:BACK", BLACK );
  printSmallNavPx( LCDWIDTH - 3 - smallNavWidth( "A:DELETE" ), 56, "A:DELETE", BLACK );
}


int main(void)
{
  uint8_t keys = 0xFF;
  uint8_t keysPrev;
  uint8_t n = 0;
  uint8_t viewRow = 0;
  uint8_t i = 0;
  uint16_t addr = 0;

  // Key interrupts priority
  PRI_KEY(0x03);

  // Enable interrupts for keys (only power)
  IRQ_ENA3 = IRQ3_KEYPOWER;

  // PRC interrupt priority
  PRI_PRC(0x01);

  // Enable PRC IRQ
  IRQ_ENA1 = IRQ1_PRC_COMPLETE;

  drawMenu();
  drawCursor( n );

  for ( ;; ) {
    keysPrev = keys;
    keys = keyScan();

    if ( curMenu == MAIN ) {
      if ( !( keys & KEY_DOWN ) && ( keys != keysPrev ) ) {
        if ( n < 2 ) {
          clearCursor( n );
          ++n;
          drawCursor( n );
        }
      }

      if ( !( keys & KEY_UP ) && ( keys != keysPrev ) ) {
        if ( n > 0 ) {
          clearCursor( n );
          --n;
          drawCursor( n );
        }
      }

      if ( !( keys & KEY_A ) && ( keys != keysPrev ) ) {
        if ( n == 0 ) {
          curMenu = VIEW;
        } else if ( n == 1 ) {
          curMenu = BACKUP;
        } else {
          curMenu = RESTORE;
        }

        n = 0;
        drawMenu();
        drawCursor( n );
      }

    } else if ( curMenu == VIEW ) {
      if ( !( keys & KEY_DOWN ) && ( keys != keysPrev ) ) {
        if ( n < EEPROMSLOTS ) {
          clearCursor( n );
          ++n;
          drawCursor( n );
        }
      }

      if ( !( keys & KEY_UP ) && ( keys != keysPrev ) ) {
        if ( n > 0 ) {
          clearCursor( n );
          --n;
          drawCursor( n );
        }
      }

      if ( !( keys & KEY_B ) && ( keys != keysPrev ) ) {
        curMenu = MAIN;
        n = 0;
        drawMenu();
        drawCursor( n );
      } else if ( !( keys & KEY_A ) && ( keys != keysPrev ) ) {
        curMenu = VIEW_RESULT;
        viewRow = 0;
        drawViewScreen( n );
        drawViewCursor( viewRow, BLACK );
      }

    } else if ( curMenu == VIEW_RESULT ) {
      if ( !( keys & KEY_DOWN ) && ( keys != keysPrev ) ) {
        if ( viewRow < ( SAVECOUNT - 1 ) ) {
          drawViewCursor( viewRow, WHITE );
          ++viewRow;
          drawViewCursor( viewRow, BLACK );
        }
      }

      if ( !( keys & KEY_UP ) && ( keys != keysPrev ) ) {
        if ( viewRow > 0 ) {
          drawViewCursor( viewRow, WHITE );
          --viewRow;
          drawViewCursor( viewRow, BLACK );
        }
      }

      if ( !( keys & KEY_B ) && ( keys != keysPrev ) ) {
        curMenu = VIEW;
        drawMenu();
        drawCursor( n );
      } else if ( n == 0 && !( keys & KEY_A ) && ( keys != keysPrev ) ) {
        readViewBytes( n, 4 + viewRow * SAVEHEADERSIZE, SAVEHEADERSIZE );
        if ( classifySaveHeader( readBuffer ) == SAVE_OCCUPIED ) {
          curMenu = DELETE_CONFIRM;
          drawDeleteScreen( n, viewRow );
        }
      }

    } else if ( curMenu == DELETE_CONFIRM ) {
      if ( ( !( keys & KEY_B ) || !( keys & KEY_A ) ) && ( keys != keysPrev ) ) {
        // B takes precedence, including when both buttons are pressed.
        if ( !( keys & KEY_A ) && ( keys & KEY_B ) && n == 0 ) {
          // No success claim: even on failure, redraw the actual EEPROM.
          // SLOT confirmations remain non-destructive.
          deleteEEPROMHeader( n, viewRow );
        }
        curMenu = VIEW_RESULT;
        drawViewScreen( n );
        drawViewCursor( viewRow, BLACK );
      }

    } else if ( curMenu == BACKUP ) {
      if ( !( keys & KEY_DOWN ) && ( keys != keysPrev ) ) {
        if ( n < ( EEPROMSLOTS - 1 ) ) {
          clearCursor( n );
          ++n;
          drawCursor( n );
        }
      }

      if ( !( keys & KEY_UP ) && ( keys != keysPrev ) ) {
        if ( n > 0 ) {
          clearCursor( n );
          --n;
          drawCursor( n );
        }
      }

      if ( !( keys & KEY_B ) && ( keys != keysPrev ) ) {
        curMenu = MAIN;
        n = 0;
        drawMenu();
        drawCursor( n );
      }

      if ( !( keys & KEY_A ) && ( keys != keysPrev ) ) {
        drawOperationScreen( BACKUP, n, 0 );

        // Set the slot.
        *( (uint8_t *) REG_SLOT ) = n;

        // Copy bytes over. 32 bytes per read.
        addr = 0;
        while ( 1 ) {
          readEEPROMBytes( addr, readBuffer, 32 );

          // Transfer the read bytes.
          for ( i = 0; i < 32; ++i ) {
            // Set lower addr.
            *( (uint8_t *) REG_LOADDR ) = ( addr & 0xFF );

            // Set upper addr.
            *( (uint8_t *) REG_HIADDR ) = ( addr >> 8 );

            // Transfer byte.
            *( (uint8_t *) REG_BYTE ) = readBuffer[ i ];

            ++addr;
          }

          if ( ( addr - 1 ) == MAXEEPROM ) {
            break;
          }
        }

        // Transfer to Flash.
        *( (uint8_t *) REG_STOREFLASH ) = 1;

        drawOperationScreen( BACKUP, n, 1 );
        waitForButton();

        curMenu = MAIN;
        n = 0;
        drawMenu();
        drawCursor( n );
      }

    } else if ( curMenu == RESTORE ) {
      if ( !( keys & KEY_DOWN ) && ( keys != keysPrev ) ) {
        if ( n < ( EEPROMSLOTS - 1 ) ) {
          clearCursor( n );
          ++n;
          drawCursor( n );
        }
      }

      if ( !( keys & KEY_UP ) && ( keys != keysPrev ) ) {
        if ( n > 0 ) {
          clearCursor( n );
          --n;
          drawCursor( n );
        }
      }

      if ( !( keys & KEY_B ) && ( keys != keysPrev ) ) {
        curMenu = MAIN;
        n = 0;
        drawMenu();
        drawCursor( n );
      }

      if ( !( keys & KEY_A ) && ( keys != keysPrev ) ) {
        drawOperationScreen( RESTORE, n, 0 );

        // Set the slot.
        *( (uint8_t *) REG_SLOT ) = n;

        // The RP2040 reloads the selected backup into CARTEEPROM when REG_SLOT
        // is written. copyToEEPROM() then writes those 8192 bytes to the
        // Pokemon Mini EEPROM in 32-byte pages.
        addr = 0;
        copyToEEPROM();

        drawOperationScreen( RESTORE, n, 1 );
        waitForButton();

        curMenu = MAIN;
        n = 0;
        drawMenu();
        drawCursor( n );
      }
    }
  }
}
