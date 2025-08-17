#include "pm.h"
#include "print.h"
#include "draw.h"
//#include "eeprom.h"

#include <stdint.h>
#include <string.h>

#define GAMELOAD ( *((volatile uint8_t _far *)0x1FFFFF) )
#define SLOTSPERPAGE 5
#define TOTALSLOTS   20
#define MAXSLOTS     30

#define CURSORX      1
#define LABELX       8
#define LABELY       15
#define LABELY_STEP  9


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

// Index of valid slots and counter
uint8_t gValidIdx[MAXSLOTS];
uint8_t gValidCount = 0;


static void rebuildMenuIndex(void) {
    uint8_t i;
    gValidCount = 0;

    for ( i=0; i < TOTALSLOTS; ++i ) {
        //Check if the slot is used
        if ( menuTitles[i][0] != '\0' ) {
            gValidIdx[gValidCount++] = i;
        }
    }

    PAGES = ( gValidCount + SLOTSPERPAGE - 1 ) / SLOTSPERPAGE;
    LASTPAGESLOTS = gValidCount % SLOTSPERPAGE;
    if (!LASTPAGESLOTS && PAGES) {
        LASTPAGESLOTS = SLOTSPERPAGE;
    }
    if ( gValidCount == 0 ) {
        PAGES = 1;
        LASTPAGESLOTS = 0;
    }
}


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


void drawMenu( uint8_t p )
{
    uint8_t i, y;
    int x_active, x, x_right_start;
    int text_x, text_y;
    int visibleIndex;
    int realIdx;

    // Tabs geometry
    const uint8_t TAB_Y          = 0;                            // Top Y position of the tabs (in pixels)
    const uint8_t TAB_H          = 11;                           // Tab height in pixels
    const uint8_t TAB_INACTIVE_W = 8;                            // Inactive tab width
    const uint8_t TAB_ACTIVE_W   = 51;                           // Active tab width
    const uint8_t TAB_OVERLAP    = 4;                            // Pixels of horizontal overlap between tabs
    const uint8_t TAB_BEVEL      = 4;                            // Bevel size (looks good between 2 and 4)
    const int     TAB_STEP       = TAB_INACTIVE_W - TAB_OVERLAP; // Horizontal advance per stacked inactive tab

    // Content frame
    const uint8_t BOTTOM         = LCDHEIGHT - 1;
    const uint8_t CONTENT_X      = 0;
    const uint8_t CONTENT_Y      = TAB_Y + TAB_H - 1;
    const uint8_t CONTENT_W      = LCDWIDTH;
    const uint8_t CONTENT_H      = BOTTOM - CONTENT_Y + 1;

    // Clear screen
    memset((void*)0x1000, 0, LCDWIDTH * (LCDHEIGHT/8));

    if ( PAGES == 1 ) {
        // Single-page mode
        x_active = 0;
        drawActiveTab(x_active, TAB_Y, TAB_ACTIVE_W - 15, TAB_H, TAB_BEVEL);
        text_x = x_active + 1;
        text_y = TAB_Y + 2;
        printPx(text_x, text_y, "GAMES", WHITE);
    } else {
        // Multi-page mode
        x_active = p * TAB_STEP;

        // Left stacked half-tabs: pages [0 .. p-1]
        i = 0;
        while (i < p) {
            x = i * TAB_STEP;
            drawHalfTabLeft(x, TAB_Y, TAB_INACTIVE_W, TAB_H);
            i++;
        }

        // Draw active full tab
        drawActiveTab(x_active, TAB_Y, TAB_ACTIVE_W, TAB_H, TAB_BEVEL);
        text_x = x_active + 1;
        text_y = TAB_Y + 2;
        printPx(text_x, text_y, "GAMES", WHITE);
        printCharPx(text_x + 34, text_y, 'P', WHITE);
        printDigitPx(text_x + 40, text_y, (unsigned char)(p + 1), WHITE);

        // Right stacked half-tabs: pages [p+1 .. PAGES-1]
        x_right_start = x_active + TAB_ACTIVE_W - TAB_OVERLAP;
        i = p + 1;
        while (i < PAGES) {
            x = x_right_start + (i - p - 1) * TAB_STEP;
            drawHalfTabRight(x, TAB_Y, TAB_INACTIVE_W, TAB_H, TAB_BEVEL);
            i++;
        }
    }

    // Draw "About" tab
    drawAboutTab(84, TAB_Y, 12, TAB_H, TAB_BEVEL, NOFILL);
    printCharPx(87, text_y, 'C', BLACK);

    // Render the game list
    i = 0;
    for ( y=LABELY ;; y += LABELY_STEP ) {
        visibleIndex = i + p * SLOTSPERPAGE;
        if (visibleIndex >= gValidCount) {
            break;
        }
        realIdx = gValidIdx[visibleIndex];
        printPx(LABELX, y, menuTitles[ realIdx ], BLACK);
        ++i;

        if ( p == PAGES - 1 ) {
            if ( i >= LASTPAGESLOTS ) {
                break;
            }
        } else {
            if ( i >= SLOTSPERPAGE ) {
                break;
            }
        }
    }

    // Inner border of the content area
    drawRect(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H, BLACK);
}


// Clears only the scrollable text area of the About screen (does not touch tabs or frame).
static void clearAboutTextArea(uint8_t startRow, uint8_t visibleRows, int startX)
{
    uint8_t screenByteRows = (uint8_t)(LCDHEIGHT / 8);
    uint8_t row;
    int col;

    for (row = 0; row < visibleRows; ++row) {
        uint8_t yRow = (uint8_t)(startRow + row);
        volatile unsigned char* p = (volatile unsigned char*)(FRAMEBUFF + startX + (uint16_t)yRow * LCDWIDTH);

        // Clear to white inside the text area width.
        for (col = startX; col < (LCDWIDTH - 1); ++col) {
            p[col - startX] = 0x00u;
        }
    }
}


void drawAboutScreenAndBlocking(void)
{
    static const char *aboutText[] = {
        "  - MADE BY -  ",
        "               ",
        " ZWENERGY  AND ",
        "    GILTESA    ",
        "       ^       ", // ▼
        "               ",
        "ZWENERGY:      ",
        "PM2040 Firmware",
        "& MultiROM Menu",
        "               ",
        "GILTESA:       ",
        "PMini Flashcard",
        "& Menu Styling ",
        "               ",
        "    2025-08    "
    };

    const uint8_t ABOUT_LINES    = (uint8_t)(sizeof(aboutText) / sizeof(aboutText[0]));

    const uint8_t FIRST_ROW      = 2;   // First byte-row inside content to draw text
    const uint8_t VISIBLE        = 5;   // How many lines are visible at once
    const int     START_X        = 2;   // Left pixel where text begins

    const uint8_t TAB_Y          = 0;
    const uint8_t TAB_H          = 11;
    const uint8_t TAB_INACTIVE_W = 8;
    const uint8_t TAB_ACTIVE_W   = 51;
    const uint8_t TAB_OVERLAP    = 4;
    const uint8_t TAB_BEVEL      = 4;
    const int     TAB_STEP       = TAB_INACTIVE_W - TAB_OVERLAP;

    // Content frame
    const uint8_t BOTTOM         = LCDHEIGHT - 1;
    const uint8_t CONTENT_X      = 0;
    const uint8_t CONTENT_Y      = TAB_Y + TAB_H - 1;
    const uint8_t CONTENT_W      = LCDWIDTH;
    const uint8_t CONTENT_H      = BOTTOM - CONTENT_Y + 1;

    uint8_t keys, keysPrev, scroll, i;

    // -------- Static header (draw once) --------
    // Clear full screen once so tab + frame remain intact afterwards.
    memset((void*)0x1000, 0, LCDWIDTH * (LCDHEIGHT / 8));

    // About tab and title
    drawAboutTab(61, TAB_Y, 35, TAB_H, TAB_BEVEL, 1);
    printPx(64, TAB_Y+2, "ABOUT", WHITE);

    // Inner content frame
    drawRect(CONTENT_X, CONTENT_Y, CONTENT_W, CONTENT_H, 1);

    // -------- Initial state --------
    keys     = keyScan();
    keysPrev = keys;
    scroll   = 0;

    // Clear text area and paint initial visible lines
    clearAboutTextArea(FIRST_ROW, VISIBLE, START_X);
    for (i = 0; i < VISIBLE && (uint8_t)(scroll + i) < ABOUT_LINES; ++i) {
        print(START_X, (int)(FIRST_ROW + i), aboutText[scroll + i], BLACK);
    }

    // -------- Input loop (line-by-line scroll) --------
    for (;;) {
        uint8_t maxScroll;

        keysPrev = keys;
        keys     = keyScan();

        // Exit on C (edge)
        if ( !(keys & KEY_C) && (keysPrev & KEY_C) ) {
            break;
        }

        // Compute scroll limits
        if (ABOUT_LINES > VISIBLE) {
            maxScroll = (uint8_t)(ABOUT_LINES - VISIBLE);
        } else {
            maxScroll = 0;
        }

        // Scroll up on UP edge
        if ( !(keys & KEY_UP) && (keysPrev & KEY_UP) ) {
            if (scroll > 0) {
                --scroll;
                clearAboutTextArea(FIRST_ROW, VISIBLE, START_X);
                for (i = 0; i < VISIBLE && (uint8_t)(scroll + i) < ABOUT_LINES; ++i) {
                    print(START_X, (int)(FIRST_ROW + i), aboutText[scroll + i], BLACK);
                }
            }
        }

        // Scroll down on DOWN edge
        if ( !(keys & KEY_DOWN) && (keysPrev & KEY_DOWN) ) {
            if (scroll < maxScroll) {
                ++scroll;
                clearAboutTextArea(FIRST_ROW, VISIBLE, START_X);
                for (i = 0; i < VISIBLE && (uint8_t)(scroll + i) < ABOUT_LINES; ++i) {
                    print(START_X, (int)(FIRST_ROW + i), aboutText[scroll + i], BLACK);
                }
            }
        }
    }
}


int main(void)
{
  uint8_t keys=0, keysPrev, curPage=0, n=0;

  // Build visible index and pages based on non-empty titles
  rebuildMenuIndex();

  // Read last status of the menu:
  //readStatusMenu(&curPage, &n); // <<<<<<<<<<<<<<<<<<<<<<< Lectura de los datos, no me importa si no se leen, pues en su definicion se definen ambos a 0 (no me importa siempre y cuando la funcion no modifique los valores salvo si se pueden leer...)

  // Key interrupts priority
  PRI_KEY(0x03);

  // Enable interrupts for keys (only power)
  IRQ_ENA3 = IRQ3_KEYPOWER;

  // PRC interrupt priority
  PRI_PRC(0x01);

  // Enable PRC IRQ
  IRQ_ENA1 = IRQ1_PRC_COMPLETE;

  drawMenu(curPage);
  printCharPx(CURSORX, LABELY + n * LABELY_STEP, '>', BLACK_ON_WHITE);

  for ( ;; ) {
    keysPrev = keys;
    keys     = keyScan();

    if ( !( keys & KEY_A ) ) {
      // Run the chosen game.
      if (gValidCount > 0) {
        //saveStatusMenu(curPage, n); //<<<<<<<<<<<<<<<<<<<<<<< guarda el estado del menu

        slotChose = gValidIdx[ n + (curPage * SLOTSPERPAGE) ];
        copyToRamEx(romStart);
      }
    }

    if ( !(keys & KEY_C) && (keys != keysPrev) ) {
      drawAboutScreenAndBlocking();
      drawMenu(curPage);
      printCharPx(CURSORX, LABELY + n * LABELY_STEP, '>', BLACK_ON_WHITE);
    }

    if ( !(keys & KEY_UP) && (keys != keysPrev) ) {
      if ( n > 0 ) {
        printCharPx(CURSORX, LABELY + n * LABELY_STEP, ' ', BLACK_ON_WHITE);
        --n;
        printCharPx(CURSORX, LABELY + n * LABELY_STEP, '>', BLACK_ON_WHITE);
      } else if ( curPage > 0 ) {

        printCharPx(CURSORX, LABELY + n * LABELY_STEP, ' ', BLACK_ON_WHITE);
        --curPage;
        n = (curPage == (PAGES - 1) ? LASTPAGESLOTS : SLOTSPERPAGE) - 1;
        drawMenu(curPage);
        printCharPx(CURSORX, LABELY + n * LABELY_STEP, '>', BLACK_ON_WHITE);
      }
    }

    if ( !(keys & KEY_DOWN) && (keys != keysPrev) ) {
      int pageMax = (curPage == (PAGES - 1)) ? (LASTPAGESLOTS - 1) : (SLOTSPERPAGE - 1);
      if ( n < pageMax ) {
        printCharPx(CURSORX, LABELY + n * LABELY_STEP, ' ', BLACK_ON_WHITE);
        ++n;
        printCharPx(CURSORX, LABELY + n * LABELY_STEP, '>', BLACK_ON_WHITE);
      } else if ( curPage < PAGES - 1 ) {

        printCharPx(CURSORX, LABELY + n * LABELY_STEP, ' ', BLACK_ON_WHITE);
        ++curPage;
        n = 0;
        drawMenu(curPage);
        printCharPx(CURSORX, LABELY + n * LABELY_STEP, '>', BLACK_ON_WHITE);
      }
    }

    if ( !(keys & KEY_RIGHT) && (keys != keysPrev) ) {
      if ( curPage < PAGES - 1 ) {
        ++curPage;
        if ( n > ((curPage == (PAGES - 1) ? LASTPAGESLOTS : SLOTSPERPAGE) - 1) )
          n = (curPage == (PAGES - 1) ? LASTPAGESLOTS : SLOTSPERPAGE) - 1;
        drawMenu(curPage);
        printCharPx(CURSORX, LABELY + n * LABELY_STEP, '>', BLACK_ON_WHITE);
      }
    }

    if ( !(keys & KEY_LEFT) && (keys != keysPrev) ) {
      if ( curPage > 0 ) {
        --curPage;
        if ( n > ((curPage == (PAGES - 1) ? LASTPAGESLOTS : SLOTSPERPAGE) - 1) )
          n = (curPage == (PAGES - 1) ? LASTPAGESLOTS : SLOTSPERPAGE) - 1;
        drawMenu(curPage);
        printCharPx(CURSORX, LABELY + n * LABELY_STEP, '>', BLACK_ON_WHITE);
      }
    }
  }
}
