#include "DRAW.h"
#include <stdint.h>


/**
 * Returns a pointer to the framebuffer byte at column x and byte-row y_byte.
 * Each framebuffer byte encodes 8 vertical pixels (bit 0 = top, bit 7 = bottom).
 *
 * @param x       X coordinate in pixels (column index).
 * @param y_byte  Vertical byte-row index (y_px >> 3).
 * @return        Volatile pointer to the framebuffer byte for (x, y_byte).
 */
static volatile unsigned char* fbPtr( int x, int y_byte ){
    // Compute linear address: FRAMEBUFF base + X offset + Y stride
    return (volatile unsigned char*)(FRAMEBUFF + x + y_byte * LCDWIDTH);
}


/**
 * Draws a fast horizontal span of pixels at row y from x1 to x2 (inclusive).
 * Each byte in the framebuffer holds 8 vertical pixels; this routine sets or
 * clears only the bit corresponding to y within those bytes.
 *
 * Optimized for horizontal lines within a single bit-plane row.
 *
 * @param x1    Starting X position in pixels.
 * @param x2    Ending X position in pixels (inclusive).
 * @param y     Y position in pixels.
 * @param color Use BLACK to set pixels (bits = 1) or WHITE to clear pixels (bits = 0).
 */
static void fillSpanFast( int x1, int x2, int y, int color )
{
    volatile unsigned char* p;
    unsigned char mask;
    int len;

    // Vertical bounds
    if ( (unsigned)y >= LCDHEIGHT ){
        return;
    }

    // Completely outside horizontally?
    if ( x2 < 0 || x1 > (LCDWIDTH - 1) ){
        return;
    }

    // Clamp to screen bounds
    if ( x1 < 0 ){ x1 = 0; }
    if ( x2 > (LCDWIDTH - 1) ){ x2 = LCDWIDTH - 1; }

    // After clamping, empty span?
    if ( x2 < x1 ){
        return;
    }

    // Pointer to first byte in the row; each byte-row is (y >> 3), bit is (y & 7)
    p = (volatile unsigned char*)(FRAMEBUFF + x1 + (y >> 3) * LCDWIDTH);

    // Bit mask for the specific pixel row inside the byte (0..7)
    mask = (unsigned char)(1u << (y & 7));

    // Number of pixels to process
    len = x2 - x1 + 1;

    if ( color == BLACK ){
        // Set bits (draw black)
        while ( len-- ){ *p++ |= mask; }
    } else { // WHITE
        // Clear bits (draw white)
        unsigned char inv = (unsigned char)~mask;
        while ( len-- ){ *p++ &= inv; }
    }
}


/**
 * Sets or clears a single pixel at the given (x_px, y_px) position.
 * Uses LSB-at-top addressing: bit 0 in a byte corresponds to the top pixel
 * of that byte row (y_px >> 3).
 *
 * @param x_px  X position in pixels.
 * @param y_px  Y position in pixels.
 * @param color Use BLACK to set pixel (bit = 1) or WHITE to clear pixel (bit = 0).
 */
void drawPixel( int x_px, int y_px, int color )
{
    volatile unsigned char* p;
    unsigned char mask;

    // Ignore if pixel is outside the display bounds
    if ( (unsigned)x_px >= LCDWIDTH || (unsigned)y_px >= LCDHEIGHT ){
        return;
    }

    // Pointer to the framebuffer byte containing the pixel
    p = fbPtr(x_px, y_px >> 3);

    // Mask for the specific pixel in the byte
    mask = (unsigned char)(1u << (y_px & 7));

    if ( color == BLACK ){
        *p |= mask;  // set bit = black pixel
    } else {
        *p &= (unsigned char)(~mask);  // clear bit = white pixel
    }
}


/**
 * Draws a horizontal line from (x1_px, y_px) to (x2_px, y_px) inclusive.
 * Optimized to modify only a single bit-plane row using one pointer walk and one precomputed mask.
 *
 * @param x1_px Starting X position in pixels.
 * @param x2_px Ending X position in pixels.
 * @param y_px  Y position in pixels.
 * @param color Use BLACK to set pixels (bit = 1) or WHITE to clear pixels (bit = 0).
 */
void drawHorLine( int x1_px, int x2_px, int y_px, int color )
{
    int x, t;
    unsigned char *p;
    unsigned char m;
    int ybyte;

    // Ignore if Y is outside vertical bounds
    if ( (unsigned)y_px >= LCDHEIGHT ){
        return;
    }

    // Swap if coordinates are reversed
    if ( x2_px < x1_px ){
        t = x1_px;
        x1_px = x2_px;
        x2_px = t;
    }

    // Clamp X coordinates to horizontal bounds
    if ( x1_px < 0 ){
        x1_px = 0;
    }
    if ( x2_px >= LCDWIDTH ){
        x2_px = LCDWIDTH - 1;
    }
    if ( x1_px > x2_px ){
        return;
    }

    // Calculate byte row and bit mask for the specified Y
    ybyte = y_px >> 3;
    m = (unsigned char)(1u << (y_px & 7));

    // Pointer to starting position in the framebuffer
    p = (unsigned char*)(FRAMEBUFF + x1_px + ybyte * LCDWIDTH);

    // Draw or erase pixels in one continuous pass
    if ( color == BLACK ){
        for (x = x1_px; x <= x2_px; ++x){
            *p++ |= m;
        }
    } else {
        unsigned char inv = (unsigned char)~m;
        for (x = x1_px; x <= x2_px; ++x){
            *p++ &= inv;
        }
    }
}


/**
 * Draws a horizontal line from (x1_px, y_px) to (x2_px, y_px) inclusive without performing bounds checks.
 * This version assumes that:
 * - x1_px..x2_px are already clipped to [0..LCDWIDTH-1]
 * - 0 <= y_px < LCDHEIGHT
 * Designed for use in tight inner loops where maximum speed is required.
 *
 * @param x1_px Starting X position in pixels (already validated).
 * @param x2_px Ending X position in pixels (already validated).
 * @param y_px  Y position in pixels (already validated).
 * @param color Use BLACK to set pixels (bit = 1) or WHITE to clear pixels (bit = 0).
 */
static void drawHorLine_NoClip( int x1_px, int x2_px, int y_px, int color )
{
    unsigned char *p = (unsigned char*)(FRAMEBUFF + x1_px + (y_px >> 3) * LCDWIDTH);
    unsigned char m  = (unsigned char)(1u << (y_px & 7));

    if ( color == BLACK ){
        while (x1_px++ <= x2_px){
            *p++ |= m;
        }
    } else {
        unsigned char inv = (unsigned char)~m;
        while (x1_px++ <= x2_px){
            *p++ &= inv;
        }
    }
}


/**
 * Draws a vertical line from (x_px, y1_px) to (x_px, y2_px) inclusive.
 * Optimized to minimize bit masking by handling the first and last partial bytes separately
 * and filling any middle full bytes directly.
 *
 * @param x_px   X position in pixels.
 * @param y1_px  Starting Y position in pixels.
 * @param y2_px  Ending Y position in pixels.
 * @param color  Use BLACK to set pixels (bit = 1) or WHITE to clear pixels (bit = 0).
 */
void drawVerLine( int x_px, int y1_px, int y2_px, int color )
{
    int t;
    unsigned char *p;
    int yb1, yb2;
    unsigned char m_first, m_last;

    // Ignore if X is outside horizontal bounds
    if ((unsigned)x_px >= LCDWIDTH){
        return;
    }

    // Swap if coordinates are reversed
    if (y2_px < y1_px){
        t = y1_px;
        y1_px = y2_px;
        y2_px = t;
    }

    // Clamp Y coordinates to vertical bounds
    if (y1_px < 0){
        y1_px = 0;
    }
    if (y2_px >= LCDHEIGHT){
        y2_px = LCDHEIGHT - 1;
    }
    if (y1_px > y2_px){
        return;
    }

    // Convert Y positions to byte row indices
    yb1 = y1_px >> 3;                  // byte row of y1
    yb2 = y2_px >> 3;                  // byte row of y2
    p = (unsigned char*)(FRAMEBUFF + x_px + yb1 * LCDWIDTH);

    if ( yb1 == yb2 ){
        // Both ends in the same byte: mask only those bits [y1..y2]
        m_first = (unsigned char)(0xFFu << (y1_px & 7));
        m_last  = (unsigned char)(0xFFu >> (7 - (y2_px & 7)));
        m_first &= m_last;
        if (color == BLACK){
            *p |=  m_first;
        } else {
            *p &= (unsigned char)~m_first;
        }
        return;
    }

    // First (partial) byte: bits from y1..7
    m_first = (unsigned char)(0xFFu << (y1_px & 7));
    if (color == BLACK){
        *p |=  m_first;
    } else {
        *p &= (unsigned char)~m_first;
    }

    // Middle full bytes (if any): all 8 bits
    if ( yb2 - yb1 > 1 ){
        int i, count = (yb2 - yb1 - 1);
        p += LCDWIDTH;
        if (color == BLACK){
            for (i = 0; i < count; ++i){
                *p = 0xFFu;
                p += LCDWIDTH;
            }
        } else {
            for (i = 0; i < count; ++i){
                *p = 0x00u;
                p += LCDWIDTH;
            }
        }
    } else {
        p += LCDWIDTH;
    }

    // Last (partial) byte: bits from 0..y2
    m_last = (unsigned char)(0xFFu >> (7 - (y2_px & 7)));
    if (color == BLACK){
        *p |=  m_last;
    } else {
        *p &= (unsigned char)~m_last;
    }
}


/**
 * Draws a vertical line from (x_px, y1_px) to (x_px, y2_px) inclusive without performing bounds checks.
 * Assumes x_px, y1_px, and y2_px are already clipped to valid display coordinates.
 * Optimized by handling the first and last partial bytes separately and filling any middle full bytes directly.
 *
 * @param x_px   X position in pixels (already validated).
 * @param y1_px  Starting Y position in pixels (already validated).
 * @param y2_px  Ending Y position in pixels (already validated).
 * @param color  Use BLACK to set pixels (bit = 1) or WHITE to clear pixels (bit = 0).
 */
static void drawVerLine_NoClip( int x_px, int y1_px, int y2_px, int color )
{
    unsigned char *p;
    int yb1 = y1_px >> 3;
    int yb2 = y2_px >> 3;
    unsigned char m1, m2;

    // Pointer to first byte row for y1_px
    p = (unsigned char*)(FRAMEBUFF + x_px + yb1 * LCDWIDTH);

    if (yb1 == yb2){
        // Both ends in the same byte: mask only bits [y1..y2]
        m1 = (unsigned char)(0xFFu << (y1_px & 7));
        m2 = (unsigned char)(0xFFu >> (7 - (y2_px & 7)));
        m1 &= m2;
        if (color == BLACK){
            *p |= m1;
        } else {
            *p &= (unsigned char)~m1;
        }
        return;
    }

    // First (partial) byte: bits from y1..7
    m1 = (unsigned char)(0xFFu << (y1_px & 7));
    if (color == BLACK){
        *p |= m1;
    } else {
        *p &= (unsigned char)~m1;
    }

    // Middle full bytes: all 8 bits set or cleared
    while (++yb1 < yb2){
        p += LCDWIDTH;
        if (color == BLACK){
            *p = 0xFFu;
        } else {
            *p = 0x00u;
        }
    }

    // Last (partial) byte: bits from 0..y2
    p += LCDWIDTH;
    m2 = (unsigned char)(0xFFu >> (7 - (y2_px & 7)));
    if (color == BLACK){
        *p |= m2;
    } else {
        *p &= (unsigned char)~m2;
    }
}


/**
 * Draws the outline of a rectangle at position (x, y) with the given width and height.
 * The edges are drawn inclusive (i.e., from x to x+w-1 and y to y+h-1).
 * The rectangle is clipped to the display bounds before drawing, and then the
 * no-clip line primitives are used for speed.
 *
 * Thin rectangles degenerate to a single vertical or horizontal line.
 *
 * @param x      X coordinate in pixels of the top-left corner.
 * @param y      Y coordinate in pixels of the top-left corner.
 * @param w      Rectangle width in pixels.
 * @param h      Rectangle height in pixels.
 * @param color  Use BLACK to set pixels (bit=1) or WHITE to clear pixels (bit=0).
 */
void drawRect( int x, int y, int w, int h, int color )
{
    int x1 = x;
    int y1 = y;
    int x2 = x + w - 1;
    int y2 = y + h - 1;

    if (w <= 0 || h <= 0){
        return;
    }

    // Clip to screen bounds
    if (x1 < 0){ x1 = 0; }
    if (y1 < 0){ y1 = 0; }
    if (x2 >= LCDWIDTH)  { x2 = LCDWIDTH  - 1; }
    if (y2 >= LCDHEIGHT){ y2 = LCDHEIGHT - 1; }

    if (x1 > x2 || y1 > y2){
        return;
    }

    // Degenerate (thin) rectangles
    if (x1 == x2){
        drawVerLine_NoClip(x1, y1, y2, color);
        return;
    }
    if (y1 == y2){
        drawHorLine_NoClip(x1, x2, y1, color);
        return;
    }

    // Top & bottom edges
    drawHorLine_NoClip(x1, x2, y1, color);
    drawHorLine_NoClip(x1, x2, y2, color);

    // Left & right edges
    drawVerLine_NoClip(x1, y1, y2, color);
    drawVerLine_NoClip(x2, y1, y2, color);
}


/**
 * Draws a filled rectangle from (x, y) with width w and height h in the specified color.
 * The rectangle is clipped to the screen bounds once, then filled efficiently by:
 *  - Filling the first partial byte row (if unaligned at the top)
 *  - Filling any middle full byte rows directly
 *  - Filling the last partial byte row (if unaligned at the bottom)
 *
 * Uses bit masks to affect only the relevant pixels in partial rows.
 *
 * @param x      X coordinate of the top-left corner in pixels.
 * @param y      Y coordinate of the top-left corner in pixels.
 * @param w      Rectangle width in pixels.
 * @param h      Rectangle height in pixels.
 * @param color  Use BLACK to set pixels or WHITE to clear pixels.
 */
void drawFillRect( int x, int y, int w, int h, int color )
{
    int x1 = x, y1 = y, x2 = x + w - 1, y2 = y + h - 1;
    int yb1, yb2;
    int xcur, br, span;

    unsigned char mask_single_row = 0;
    unsigned char inv_mask_single_row = 0;
    unsigned char m1 = 0, inv1 = 0;
    unsigned char m2 = 0, inv2 = 0;
    unsigned char val = 0;

    unsigned char *p;

    if (w <= 0 || h <= 0){
        return;
    }

    // Clip a pantalla
    if (x1 < 0){ x1 = 0; }
    if (y1 < 0){ y1 = 0; }
    if (x2 >= LCDWIDTH)  { x2 = LCDWIDTH  - 1; }
    if (y2 >= LCDHEIGHT){ y2 = LCDHEIGHT - 1; }
    if (x1 > x2 || y1 > y2){
        return;
    }

    yb1 = (y1 >> 3);   // primera fila de bytes
    yb2 = (y2 >> 3);   // última fila de bytes

    if (yb1 == yb2){
        // Todo el recto cabe en una única fila de bytes
        mask_single_row = (unsigned char)((0xFFu << (y1 & 7)) & (0xFFu >> (7 - (y2 & 7))));
        inv_mask_single_row = (unsigned char)(~mask_single_row);

        for (xcur = x1; xcur <= x2; ++xcur){
            p = (unsigned char*)(FRAMEBUFF + xcur + yb1 * LCDWIDTH);
            if (color){
                *p |= mask_single_row;
            } else {
                *p &= inv_mask_single_row;
            }
        }
        return;
    }

    // Primera fila parcial
    m1   = (unsigned char)(0xFFu << (y1 & 7));
    inv1 = (unsigned char)(~m1);
    for (xcur = x1; xcur <= x2; ++xcur){
        p = (unsigned char*)(FRAMEBUFF + xcur + yb1 * LCDWIDTH);
        if (color){
            *p |= m1;
        } else {
            *p &= inv1;
        }
    }

    // Filas completas intermedias (si las hay)
    if (yb2 - yb1 > 1){
        val = (unsigned char)(color ? 0xFFu : 0x00u);
        for (br = yb1 + 1; br <= yb2 - 1; ++br){
            p = (unsigned char*)(FRAMEBUFF + x1 + br * LCDWIDTH);
            span = x2 - x1 + 1;
            while (span--){
                *p++ = val;
            }
        }
    }

    // Última fila parcial
    m2   = (unsigned char)(0xFFu >> (7 - (y2 & 7)));
    inv2 = (unsigned char)(~m2);
    p = (unsigned char*)(FRAMEBUFF + x1 + yb2 * LCDWIDTH);
    for (xcur = x1; xcur <= x2; ++xcur){
        if (color){
            *p |= m2;
        } else {
            *p &= inv2;
        }
        ++p;
    }
}


/**
 * Draws a straight line from (x0, y0) to (x1, y1) in any direction using
 * the Bresenham line algorithm. Supports horizontal, vertical, and diagonal lines.
 * Plots each pixel individually with drawPixel().
 *
 * @param x0 The starting X coordinate.
 * @param y0 The starting Y coordinate.
 * @param x1 The ending X coordinate.
 * @param y1 The ending Y coordinate.
 * @param color 1 to set pixels (black), 0 to clear pixels (white).
 */
void drawLine( int x0, int y0, int x1, int y1, int color )
{
    int dx, sx, dy, sy, err, e2;

    /* Calculate absolute deltas */
    dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    sx = (x0 < x1) ? 1 : -1;
    dy = (y1 > y0) ? (y1 - y0) : (y0 - y1);
    sy = (y0 < y1) ? 1 : -1;

    /* Initialize error term */
    err = (dx > dy ? dx : -dy) / 2;

    for (;;){
        drawPixel(x0, y0, color); /* Plot current pixel */
        if (x0 == x1 && y0 == y1){
            break;
        }
        e2 = err;
        if (e2 > -dx){
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy){
            err += dx;
            y0 += sy;
        }
    }
}


/**
 * Fills a rectangular area with vertical hatched stripes.
 * Stripes are spaced horizontally by the given step in pixels.
 *
 * @param x     X coordinate of the top-left corner.
 * @param y     Y coordinate of the top-left corner.
 * @param w     Rectangle width in pixels.
 * @param h     Rectangle height in pixels.
 * @param step  Horizontal spacing between vertical stripes in pixels.
 *              Minimum 1; if <= 0, a default spacing of 2 is used.
 */
void fillRectHatched( int x, int y, int w, int h, int step )
{
    int xx, y2, xend;

    if (w <= 0 || h <= 0){
        return;
    }
    if (step <= 0){
        step = 2;
    }

    y2   = y + h - 1;
    xend = x + w;

    /* Draw vertical stripes across the rectangle */
    for (xx = x; xx < xend; xx += step){
        drawVerLine(xx, y, y2, BLACK);
    }
}


/* UI helper */


/**
 * Draws a beveled tab (UI element) with an optional diagonal bevel at the top-right corner.
 * The bevel size is clamped to fit within the tab dimensions.
 * The inside is filled (solid) up to the bevel boundary, leaving the bevel area unfilled.
 * The border is always drawn in black.
 *
 * @param x      X coordinate of the top-left corner.
 * @param y      Y coordinate of the top-left corner.
 * @param w      Width of the tab in pixels.
 * @param h      Height of the tab in pixels.
 * @param bevel  Bevel size in pixels. If 0, the tab has square corners.
 */
void drawActiveTab( int x, int y, int w, int h, int bevel )
{
    int x2, y2;

    if (w <= 0 || h <= 0){
        return;
    }
    if (bevel < 0){
        bevel = 0;
    }

    // Clamp so the diagonal fits
    if (bevel > w - 1){
        bevel = w - 1;
    }
    if (bevel > h - 1){
        bevel = h - 1;
    }

    x2 = x + w - 1;
    y2 = y + h - 1;

    // Fill inside, clipped exactly to the bevel (fast)
    if (w > 2 && h > 2){
        int yy;
        const int innerL = x + 1;
        const int innerR = x2 - 1;
        const int bevelLimit = y + bevel;

        for (yy = y + 1; yy <= y2 - 1; ++yy){
            int xr;

            if (bevel > 0 && yy <= bevelLimit){
                // Diagonal from (x2 - bevel, y) to (x2, y + bevel)
                int xdiag = (x2 - bevel) + (yy - y);
                xr = xdiag - 1;
                if (xr > innerR){
                    xr = innerR; /* safety */
                }
            } else {
                xr = innerR;
            }

            if (xr >= innerL){
                fillSpanFast(innerL, xr, yy, BLACK);
            }
        }
    }

    // Borders (always black)
    drawVerLine(x, y, y2, BLACK);                      // left side

    if (bevel > 0){
        drawHorLine(x, x2 - bevel, y, BLACK);          // top until bevel
        drawLine(x2 - bevel, y, x2, y + bevel, BLACK); // bevel diagonal
        drawVerLine(x2, y + bevel, y2, BLACK);         // right side below bevel
    } else {
        drawHorLine(x, x2, y, BLACK);                  // full top
        drawVerLine(x2, y, y2, BLACK);                 // full right side
    }

    drawHorLine(x, x2, y2, BLACK);                     // bottom edge
}


/**
 * Draws a "half tab" shape with a flat vertical edge on the left side.
 * The outline is always drawn in black and no fill is applied inside.
 *
 * @param x The X coordinate of the top-left corner.
 * @param y The Y coordinate of the top-left corner.
 * @param w The width of the tab in pixels.
 * @param h The height of the tab in pixels.
 */
void drawHalfTabLeft( int x, int y, int w, int h )
{
    int x2 = x + w - 1;
    int y2 = y + h - 1;

    if ( w <= 0 || h <= 0 ){
        return;
    }

    drawVerLine(x, y,  y2, BLACK); // Left
    drawHorLine(x, x2, y,  BLACK); // Top
    drawHorLine(x, x2, y2, BLACK); // Bottom
}


/**
 * Draws a "half tab" shape with a flat vertical edge on the right side.
 * Optionally adds a bevel to the top-right corner. The outline is always black.
 *
 * @param x The X coordinate of the top-left corner.
 * @param y The Y coordinate of the top-left corner.
 * @param w The width of the tab in pixels.
 * @param h The height of the tab in pixels.
 * @param bevel The bevel size in pixels. If 0, the corner is square.
 */
void drawHalfTabRight( int x, int y, int w, int h, int bevel )
{
    int x2, y2;

    if (w <= 0 || h <= 0) return;
    if (bevel < 0) bevel = 0;

    // Clamp bevel
    if ( bevel > w - 1 ){
        bevel = w - 1;
    }
    if ( bevel > h - 1 ){
        bevel = h - 1;
    }

    x2 = x + w - 1;
    y2 = y + h - 1;

    // Borders (always black)
    if (bevel > 0){
        drawHorLine(x, x2 - bevel, y, BLACK);          // Top until bevel
        drawLine(x2 - bevel, y, x2, y + bevel, BLACK); // Bevel
        drawVerLine(x2, y + bevel, y2, BLACK);         // Right side below bevel
    } else {
        drawHorLine(x, x2, y, BLACK);
        drawVerLine(x2, y, y2, BLACK);
    }

    drawHorLine(x, x2, y2, BLACK); // Bottom
}


/**
 * Draws an "about tab" shape with an optional bevel at the top-left corner.
 * The bevel size is clamped to fit within the tab dimensions.
 * The outline is always drawn in black.
 * Optionally, the inside of the tab can be filled in black, respecting the bevel shape.
 *
 * @param x     The X coordinate of the top-left corner.
 * @param y     The Y coordinate of the top-left corner.
 * @param w     The width of the tab in pixels.
 * @param h     The height of the tab in pixels.
 * @param bevel The bevel size in pixels. If 0, the tab has square corners.
 * @param fill  If non-zero, the inside of the tab is filled in black; if zero, only the outline is drawn.
 */
void drawAboutTab( int x, int y, int w, int h, int bevel, int fill )
{
    int x2, y2;

    if (w <= 0 || h <= 0) return;
    if (bevel < 0) bevel = 0;

    // Clamp so the diagonal fits
    if (bevel > w - 1) bevel = w - 1;
    if (bevel > h - 1) bevel = h - 1;

    x2 = x + w - 1;
    y2 = y + h - 1;

    // Fill inside, clipped to the top-left bevel
    if (fill && w > 2 && h > 2){
        int yy;
        const int innerR = x2 - 1;          // right interior
        const int innerL = x  + 1;          // left interior baseline
        const int bevelLimit = y + bevel;   // last row affected by bevel

        for (yy = y + 1; yy <= y2 - 1; ++yy){
            int xl;

            if (bevel > 0 && yy <= bevelLimit){
                // Diagonal from (x, y + bevel) to (x + bevel, y).
                // For this row: xdiag = (x + bevel) - (yy - y).
                // Fill from xdiag + 1 so we never overwrite the bevel pixel.
                int xdiag = (x + bevel) - (yy - y);
                xl = xdiag + 1;
                if (xl < innerL) xl = innerL;   // safety
            } else {
                xl = innerL;
            }

            if (xl <= innerR){
                fillSpanFast(xl, innerR, yy, BLACK);
            }
        }
    }

    // --- Borders (always black) ---
    if ( bevel > 0 ){
        drawLine(x, y + bevel, x + bevel, y, BLACK); // Bevel (top-left corner)
        drawHorLine(x + bevel, x2, y, BLACK);        // Top edge after bevel
        drawVerLine(x2, y, y2, BLACK);               // Right vertical edge
    } else {
        // No bevel: full top and right
        drawHorLine(x, x2, y, BLACK);
        drawVerLine(x2, y, y2, BLACK);
    }

    // Left side below bevel
    if (bevel > 0){
        drawVerLine(x, y + bevel, y2, BLACK);
    } else {
        drawVerLine(x, y, y2, BLACK);
    }

    // Bottom edge
    drawHorLine(x, x2, y2, BLACK);
}

