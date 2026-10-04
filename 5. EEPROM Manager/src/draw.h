#ifndef __DRAW_H
#define __DRAW_H

#ifndef FRAMEBUFF
    #define FRAMEBUFF 0x1000
#endif
#ifndef LCDWIDTH
    #define LCDWIDTH  96
#endif
#ifndef LCDHEIGHT
    #define LCDHEIGHT 64
#endif

#ifndef WHITE
    #define WHITE      0
#endif
#ifndef BLACK
    #define BLACK      1
#endif
#ifndef NOFILL
    #define NOFILL     0
#endif
#ifndef FILL
    #define FILL       1
#endif


void drawPixel( int x, int y, int color );
void drawHorLine( int x1, int x2, int y, int color );
void drawVerLine( int x, int y1, int y2, int color );
void drawRect( int x, int y, int w, int h, int color );
void drawFillRect( int x, int y, int w, int h, int color );
void drawLine( int x0, int y0, int x1, int y1, int color );
void fillRectHatched( int x, int y, int w, int h, int step );

/* UI helper */
void drawActiveTab( int x, int y, int w, int h, int bevel );
void drawHalfTabLeft( int x, int y, int w, int h );
void drawHalfTabRight( int x, int y, int w, int h, int bevel );
void drawAboutTab( int x, int y, int w, int h, int bevel, int fill );

#endif