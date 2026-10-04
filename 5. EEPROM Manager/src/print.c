#include "font_6x8.h"
#include "print.h"


/**
 * Checks if an X coordinate in pixels is within the valid LCD display range.
 *
 * @param x_px X coordinate in pixels.
 * @return Non-zero if inside bounds, 0 otherwise.
 */
static int inBoundsX( int x_px ) {
    return (x_px >= 0 && x_px < LCDWIDTH);
}


/**
 * Returns a pointer to the 6x8 glyph for a given ASCII character.
 *
 * @param c ASCII character code.
 * @return Pointer to the glyph data in font6x8 (6 bytes, one per column).
 */
static const unsigned char* glyphPtr( unsigned char c ) {
    if ( c == ' ' ) {
        return font6x8[0];
    }

    if ( c == '.' ) {
        return font6x8[1];
    }

    if ( c >= '0' && c <= '9' ) {
        return font6x8[2 + ( c - '0' )];
    }

    if ( c >= ':' && c <= '@' ) {
        return font6x8[12 + ( c - ':' )];
    }

    if ( c >= 'A' && c <= 'Z' ) {
        return font6x8[19 + ( c - 'A' )];
    }

    // Unsupported characters are shown as '?'.
    return font6x8[17];
}


/**
 * Fills the 6×8 character cell background at a pixel position.
 * Works for any vertical alignment (y_px may straddle two byte rows).
 *
 * @param x_px  X coordinate in pixels (left edge of the cell).
 * @param y_px  Y coordinate in pixels (top of the cell).
 * @param color 0 = white (clear bits), 1 = black (set bits).
 */
static void fillCharCell( int x_px, int y_px, int color ) {
    int i;
    int y_byte = (y_px >> 3);
    int shift  = (y_px & 7);

    if (y_px < 0 || y_px >= LCDHEIGHT) {
        return;
    }
    if (!inBoundsX(x_px) || !inBoundsX(x_px + CHARWIDTH - 1)) {
        return;
    }

    if (shift == 0) {
        volatile unsigned char* p = (volatile unsigned char*)(FRAMEBUFF + x_px + y_byte * LCDWIDTH);
        unsigned char val = (unsigned char)(color ? 0xFFu : 0x00u);
        for (i = 0; i < CHARWIDTH; ++i) {
            p[i] = val;
        }
    } else {
        volatile unsigned char* p0 = (volatile unsigned char*)(FRAMEBUFF + x_px + y_byte * LCDWIDTH);
        volatile unsigned char* p1 = (y_byte < (LCDHEIGHT / 8 - 1))
                                     ? (volatile unsigned char*)(FRAMEBUFF + x_px + (y_byte + 1) * LCDWIDTH)
                                     : 0;

        unsigned char topMask = (unsigned char)(0xFFu << shift);
        unsigned char botMask = (unsigned char)(0xFFu >> (8 - shift));

        if (color) {
            // Black fill: set covered bits in both rows
            for (i = 0; i < CHARWIDTH; ++i) {
                p0[i] |= topMask;
            }
            if (p1) {
                for (i = 0; i < CHARWIDTH; ++i) {
                    p1[i] |= botMask;
                }
            }
        } else {
            // White fill: clear covered bits in both rows
            unsigned char invTop = (unsigned char)~topMask;
            unsigned char invBot = (unsigned char)~botMask;

            for (i = 0; i < CHARWIDTH; ++i) {
                p0[i] &= invTop;
            }
            if (p1) {
                for (i = 0; i < CHARWIDTH; ++i) {
                    p1[i] &= invBot;
                }
            }
        }
    }
}


/**
 * Renders a single 6x8 character at an arbitrary pixel position (x_px, y_px)
 * using the specified color mode.
 *
 * @param x_px     X coordinate in pixels.
 * @param y_px     Y coordinate in pixels.
 * @param c        Character to render.
 * @param color    Rendering mode: WHITE, WHITE_ON_BLACK, BLACK, BLACK_ON_WHITE.
 */
void printCharPx( int x_px, int y_px, unsigned char c, int color ) {
    int i;
    int y_byte = y_px >> 3;
    int shift  = y_px & 7;
    const unsigned char* fontP = glyphPtr(c);

    if (y_px < 0 || y_px >= LCDHEIGHT) return;
    if (!inBoundsX(x_px) || !inBoundsX(x_px + CHARWIDTH - 1)) return;

    // Background fill for ON_* modes
    if (color == WHITE_ON_BLACK) {
        fillCharCell(x_px, y_px, 1);
    } else if (color == BLACK_ON_WHITE) {
        fillCharCell(x_px, y_px, 0);
    }

    if (shift == 0) {
        volatile unsigned char* p = (volatile unsigned char*)(FRAMEBUFF + x_px + y_byte * LCDWIDTH);
        for (i = 0; i < CHARWIDTH; ++i) {
            unsigned char col = fontP[i];
            if (color == BLACK || color == BLACK_ON_WHITE) {
                p[i] |= col; // black text
            } else {
                p[i] &= (unsigned char)(~col); // white text
            }
        }
    } else {
        volatile unsigned char* p0 = (volatile unsigned char*)(FRAMEBUFF + x_px + y_byte * LCDWIDTH);
        volatile unsigned char* p1 = (y_byte < (LCDHEIGHT / 8 - 1))
                                     ? (volatile unsigned char*)(FRAMEBUFF + x_px + (y_byte + 1) * LCDWIDTH)
                                     : 0;

        for (i = 0; i < CHARWIDTH; ++i) {
            unsigned char col = fontP[i];
            unsigned char top = (unsigned char)(col << shift);
            unsigned char bot = (unsigned char)(col >> (8 - shift));

            if (color == BLACK || color == BLACK_ON_WHITE) {
                if (p0) p0[i] |= top;
                if (p1) p1[i] |= bot;
            } else {
                if (p0) p0[i] &= (unsigned char)(~top);
                if (p1) p1[i] &= (unsigned char)(~bot);
            }
        }
    }
}


/**
 * Renders a numeric digit (0–9) at an arbitrary pixel position.
 *
 * @param x_px  X coordinate in pixels.
 * @param y_px  Y coordinate in pixels.
 * @param c     Digit to render (0–9).
 * @param color Rendering mode.
 */
void printDigitPx( int x_px, int y_px, unsigned char c, int color ) {
    printCharPx(x_px, y_px, c + 48, color);
}


/**
 * Renders a null-terminated string at an arbitrary pixel position.
 * Drawing stops when the next character would start beyond the right edge
 * of the display (only full 6-pixel wide characters are drawn).
 *
 * @param x_px  Starting X position in pixels.
 * @param y_px  Starting Y position in pixels.
 * @param str   Null-terminated string to render.
 * @param color Rendering mode.
 */
void printPx( int x_px, int y_px, const char* str, int color ) {
    while (*str) {
        if (x_px > (LCDWIDTH - CHARWIDTH)) break;
        printCharPx(x_px, y_px, (unsigned char)*str, color);
        x_px += CHARWIDTH;
        ++str;
    }
}