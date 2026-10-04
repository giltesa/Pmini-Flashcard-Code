#ifndef __H
#define __H

#ifndef FRAMEBUFF
    #define FRAMEBUFF 0x1000
#endif
#ifndef LCDWIDTH
    #define LCDWIDTH  96
#endif
#ifndef LCDHEIGHT
    #define LCDHEIGHT 64
#endif
#ifndef CHARWIDTH
    #define CHARWIDTH 6
#endif

// Color modes for text rendering
#ifndef WHITE
    #define WHITE      0  // white text, background untouched
#endif
#ifndef BLACK
    #define BLACK      1  // black text, background untouched
#endif
#define WHITE_ON_BLACK 2  // white text on black background
#define BLACK_ON_WHITE 3  // black text on white background

// Pixel-aligned versions
void printCharPx( int x_px, int y_px, unsigned char c, int color );
void printDigitPx( int x_px, int y_px, unsigned char c, int color );
void printPx( int x_px, int y_px, const char* str, int color );

#endif
