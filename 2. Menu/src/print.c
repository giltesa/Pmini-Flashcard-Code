#include "font_6x8.h"
#include "print.h"

#define FRAMEBUFF 0x1000
#define CHARWIDTH 6
#define LCDWIDTH 96

void printChar( int x, int y, unsigned char c ) {
  unsigned char* buffP;
  const unsigned char* fontP;
  int i;  
  
  buffP = ( unsigned char* ) ( FRAMEBUFF + x + y * LCDWIDTH );
  
  // Unsupported chars/white space.
  if ( c < 32 ) {
    c = 32;
    
  } else if ( c >= 32 && c < 91 ) {
    // No change.
    
  } else if ( c >= 65 && c < 123 ) {
    // Convert small to capital.
    c -= 32;
    
  } else {
    // Return "?"
    c = 63;
  }
  
  // Remove the base.
  c -= 32;
  
  fontP = font6x8[ c ];
  
  // Print the single slivers.
  for ( i = 0; i < CHARWIDTH; ++i ) {
    // Copy the sliver into the frame buffer.
    *buffP++ = *fontP++;
  }
}

void printDigit( int x, int y, unsigned char c ) {
  printChar( x, y, c + 48 );
}

void print( int x, int y, const char* str ) {
  for ( ; *str; ++str ) {
    printChar( x, y, *str );
    x += CHARWIDTH;
  }
}

