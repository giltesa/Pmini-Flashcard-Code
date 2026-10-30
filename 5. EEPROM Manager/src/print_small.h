#ifndef PRINT_SMALL_H
#define PRINT_SMALL_H

// Transparent five-pixel text, six-pixel advance. BLACK or WHITE only.
void printSmallPx( int x_px, int y_px, const char* str, int color );

// Navigation only: six-pixel advance, except a trimmed colon at three pixels.
int smallNavWidth( const char* str );
void printSmallNavPx( int x_px, int y_px, const char* str, int color );

#endif
