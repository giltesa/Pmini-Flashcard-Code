#include "pm.h"
#include "print.h"

#include <stdint.h>
#include <string.h>

#define GAMELOAD ( *((volatile uint8_t _far *)0x1FFFFF) )
#define SLOTSPERPAGE 5
#define TOTALSLOTS 2
#define MAXSLOTS 30

#define PAGEX 90
#define PAGEY 7
#define indOffset 2

#define CURSORX 1
#define LABELX 8

uint8_t ram[1024];
uint8_t slotChose;
volatile uint8_t flag;
uint8_t PAGES, LASTPAGESLOTS;

char menuTitles[ MAXSLOTS ][ 21 ] = {
  "SLOT 1", "SLOT 2", "SLOT 3", "SLOT 4", "SLOT 5", 
  "SLOT 6", "SLOT 7", "SLOT 8", "SLOT 9", "SLOT A", 
  "SLOT B", "SLOT C", "SLOT D", "SLOT E", "SLOT F", 
  "SLOT G", "SLOT H", "SLOT I", "SLOT J", "SLOT K",
  "SLOT L", "SLOT M", "SLOT N", "SLOT O", "SLOT P", 
  "SLOT Q", "SLOT R", "SLOT S", "SLOT T", "SLOT U" };

static void copyToRamEx( void ( *fOrig )( void ) ) {
    void ( *fRam )( void );
    uint8_t* p;

    p = ( void* )fOrig;
    fRam = ( void* ) ram;
    
    // Copy funtion to RAM.
    memcpy( ram, p, sizeof( ram ) );

    // Execute.
    ( *fRam )();
}

static void romStart( void ) {
  // Write to special memory.
  GAMELOAD = slotChose;

  // Reset.
  _int( 0x02 );
}

static uint8_t keyScan( void ) {
  uint8_t k = KEY_PAD;
  
  return k;
}


_interrupt( 2 ) void prc_frame_copy_irq(void)
{
  flag = 1;
  IRQ_ACT1 = IRQ1_PRC_COMPLETE;
}

void drawMenu( uint8_t p ) {
  uint8_t i = indOffset;
  
  // Empty screen.
  memset( 0x1000, 0, 96 * 8 );

  print( 10, 0, "PM2040 X-in-1" );
  for ( ;; ) {
    print( LABELX, i, menuTitles[ i - indOffset + p * SLOTSPERPAGE ] );
    ++i;
    
    if ( p == PAGES - 1 ) {
      if ( i >= LASTPAGESLOTS + indOffset ) {
        break;
      }
    } else {
      if ( i >= SLOTSPERPAGE + indOffset ) {
        break;
      }
    }
  }

  printDigit( PAGEX, PAGEY, p );
} 

int main(void)
{
  uint8_t keys, keysPrev, curPage, n;
  uint8_t cnt = 0;

  // Set number of pages.
  PAGES = ( TOTALSLOTS + SLOTSPERPAGE - 1) / SLOTSPERPAGE;
  LASTPAGESLOTS = TOTALSLOTS % SLOTSPERPAGE;
  if ( !LASTPAGESLOTS )
    LASTPAGESLOTS = SLOTSPERPAGE;

  curPage = 0;

  // Key interrupts priority
  PRI_KEY(0x03);

  // Enable interrupts for keys (only power)
  IRQ_ENA3 = IRQ3_KEYPOWER;

  // PRC interrupt priority
  PRI_PRC(0x01);

  // Enable PRC IRQ
  IRQ_ENA1 = IRQ1_PRC_COMPLETE;

  n = indOffset;

  drawMenu( 0 );
  print( CURSORX, n, ">" );

  for ( ;; ) {
    keysPrev = keys;
    keys = keyScan();

    if ( !( keys & KEY_A ) ) {
      // Run the chosen game.
      slotChose = ( ( n - indOffset ) + ( curPage * SLOTSPERPAGE ) );
      copyToRamEx( romStart );
    }
    
    if ( !( keys & KEY_UP ) && ( keys != keysPrev ) ) {
      if ( n > indOffset ){
        print( CURSORX, n, " " );
        --n;
        print( CURSORX, n, ">" );

      } 
    }
    
    if ( !( keys & KEY_DOWN ) && ( keys != keysPrev  ) ) {
      if ( ( curPage != ( PAGES - 1 ) && n < SLOTSPERPAGE + 1 ) ||
           ( curPage == ( PAGES - 1 ) && n < LASTPAGESLOTS + 1 ) ){
        print( CURSORX, n, " " );
        ++n;
        print( CURSORX, n, ">" );
      }
    }

    if ( !( keys & KEY_RIGHT ) && ( keys != keysPrev  ) ) {
      if ( curPage < PAGES - 1 ) {
        ++curPage;
        drawMenu( curPage );
        print( CURSORX, n, ">" );

      }
    }

    if ( !( keys & KEY_LEFT ) && ( keys != keysPrev  ) ) {
      if (curPage > 0 ) {
        --curPage;
        drawMenu( curPage );
        print( CURSORX, n, ">" );
      }
    }
  }
}
