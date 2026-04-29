#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "gba.h"

/* Fill entire screen with one color */
void gba_clearScreen(u16 color);

/* Set a single pixel (bounds-checked via unsigned cast) */
void gba_setPixel(int x, int y, u16 color);

/* Fill a solid rectangle */
void gba_fillRect(int x, int y, int w, int h, u16 color);

/* Draw outline rectangle (1-pixel border) */
void gba_drawRect(int x, int y, int w, int h, u16 color);

/* Draw a horizontal line */
void gba_drawHLine(int x, int y, int w, u16 color);

/* Draw a character from the 5x7 font; returns x-advance */
int  gba_drawChar(int x, int y, char c, u16 fg, u16 bg);

/* Draw a null-terminated ASCII string */
void gba_drawString(int x, int y, const char *str, u16 fg, u16 bg);

/* Draw a non-negative decimal integer */
void gba_drawNumber(int x, int y, int num, u16 fg, u16 bg);

/* Pixel dimensions of a string (for centering) */
int  gba_stringWidth(const char *str);

#endif /* GRAPHICS_H */
