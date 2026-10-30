#include "font_5x5.h"
#include "print.h"
#include "print_small.h"
#include "draw.h"


static const unsigned char* glyphPtr( unsigned char c ) {
    if ( c >= ' ' && c <= '`' ) {
        return font5x5[c - ' '];
    }

    if ( c >= '{' && c <= '~' ) {
        return font5x5[65 + ( c - '{' )];
    }

    return font5x5['?' - ' '];
}


void printSmallPx( int x_px, int y_px, const char* str, int color ) {
    int i;
    int y_byte = y_px >> 3;
    int shift = y_px & 7;
    unsigned char col;
    const unsigned char* fontP;
    volatile unsigned char* p;

    // Draw only complete glyphs. The separator column remains untouched.
    if ( y_px < 0 || y_px > LCDHEIGHT - 5 ) {
        return;
    }

    while ( *str ) {
        if ( x_px < 0 || x_px > LCDWIDTH - 5 ) {
            break;
        }

        fontP = glyphPtr( (unsigned char) *str );
        p = (volatile unsigned char*)( FRAMEBUFF + x_px + y_byte * LCDWIDTH );
        for ( i = 0; i < 5; ++i ) {
            col = fontP[ i ];
            if ( color == BLACK ) {
                p[ i ] |= (unsigned char)( col << shift );
            } else {
                p[ i ] &= (unsigned char) ~( col << shift );
            }

            if ( shift > 3 ) {
                if ( color == BLACK ) {
                    p[ i + LCDWIDTH ] |= (unsigned char)( col >> ( 8 - shift ) );
                } else {
                    p[ i + LCDWIDTH ] &= (unsigned char) ~( col >> ( 8 - shift ) );
                }
            }
        }

        x_px += 6;
        ++str;
    }
}


int smallNavWidth( const char* str ) {
    int width = 0;
    int trailing = 0;

    while ( *str ) {
        width += ( *str == ':' ) ? 3 : 6;
        trailing = ( *str == ':' ) ? 2 : 1;
        ++str;
    }

    return width - trailing;
}


void printSmallNavPx( int x_px, int y_px, const char* str, int color ) {
    char glyph[ 2 ];
    int row;
    unsigned char col;

    if ( y_px < 0 || y_px > LCDHEIGHT - 5 ) {
        return;
    }

    glyph[ 1 ] = 0;
    while ( *str ) {
        if ( x_px < 0 || x_px > LCDWIDTH - ( *str == ':' ? 1 : 5 ) ) {
            break;
        }

        if ( *str == ':' ) {
            // The existing colon has ink only in its middle column.
            col = glyphPtr( ':' )[ 2 ];
            for ( row = 0; row < 5; ++row ) {
                if ( col & ( 1 << row ) ) {
                    drawPixel( x_px, y_px + row, color );
                }
            }
            x_px += 3;
        } else {
            glyph[ 0 ] = *str;
            printSmallPx( x_px, y_px, glyph, color );
            x_px += 6;
        }
        ++str;
    }
}
