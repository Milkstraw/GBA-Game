#include <tonc.h>
#include "hud.h"
#include "tile_engine.h"
#include "inventory.h"
#include "daytimer.h"

/*
 * Minimal 1bpp 8×8 pixel font encoded as 8 × u8 row masks.
 * Index order: space, 0-9, A-Z, $, ., :, /, +, -
 * Each byte represents one pixel row (bit7=leftmost pixel).
 * Rendered into 4bpp tiles: lit pixel → palette index 1, off → index 0.
 */

/* 43 glyphs × 8 rows */
static const u8 k_font_bitmaps[43][8] = {
    /* 0: space */    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 1: '0' */      {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00},
    /* 2: '1' */      {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
    /* 3: '2' */      {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00},
    /* 4: '3' */      {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
    /* 5: '4' */      {0x0E,0x1E,0x36,0x66,0x7F,0x06,0x06,0x00},
    /* 6: '5' */      {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
    /* 7: '6' */      {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00},
    /* 8: '7' */      {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
    /* 9: '8' */      {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},
    /* 10: '9' */     {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00},
    /* 11: 'A' */     {0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00},
    /* 12: 'B' */     {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00},
    /* 13: 'C' */     {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00},
    /* 14: 'D' */     {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00},
    /* 15: 'E' */     {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00},
    /* 16: 'F' */     {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00},
    /* 17: 'G' */     {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00},
    /* 18: 'H' */     {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00},
    /* 19: 'I' */     {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
    /* 20: 'J' */     {0x1E,0x06,0x06,0x06,0x66,0x66,0x3C,0x00},
    /* 21: 'K' */     {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00},
    /* 22: 'L' */     {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00},
    /* 23: 'M' */     {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00},
    /* 24: 'N' */     {0x66,0x76,0x7E,0x6E,0x66,0x66,0x66,0x00},
    /* 25: 'O' */     {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    /* 26: 'P' */     {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00},
    /* 27: 'Q' */     {0x3C,0x66,0x66,0x66,0x6E,0x3C,0x06,0x00},
    /* 28: 'R' */     {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00},
    /* 29: 'S' */     {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},
    /* 30: 'T' */     {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
    /* 31: 'U' */     {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00},
    /* 32: 'V' */     {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00},
    /* 33: 'W' */     {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    /* 34: 'X' */     {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00},
    /* 35: 'Y' */     {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00},
    /* 36: 'Z' */     {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00},
    /* 37: '$' */     {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00},
    /* 38: '.' */     {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
    /* 39: ':' */     {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00},
    /* 40: '/' */     {0x02,0x06,0x0C,0x18,0x30,0x60,0x40,0x00},
    /* 41: '+' */     {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00},
    /* 42: '-' */     {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00},
};

/* ----------------------------------------------------------------
 * hud_char_to_glyph — map ASCII to glyph index (0-based within k_font_bitmaps)
 * ---------------------------------------------------------------- */
static u16 hud_char_to_tile(char c)
{
    if (c == ' ')                 return HUD_GLYPH_SPACE;
    if (c >= '0' && c <= '9')    return (u16)(HUD_GLYPH_DIGIT0 + (c - '0'));
    if (c >= 'A' && c <= 'Z')    return (u16)(HUD_GLYPH_A + (c - 'A'));
    if (c >= 'a' && c <= 'z')    return (u16)(HUD_GLYPH_A + (c - 'a'));  /* fold to upper */
    if (c == '$') return HUD_GLYPH_DOLLAR;
    if (c == '.') return HUD_GLYPH_DOT;
    if (c == ':') return HUD_GLYPH_COLON;
    if (c == '/') return HUD_GLYPH_SLASH;
    if (c == '+') return HUD_GLYPH_PLUS;
    if (c == '-') return HUD_GLYPH_MINUS;
    return HUD_GLYPH_SPACE;  /* unknown → space */
}

/* ----------------------------------------------------------------
 * Upload a 1bpp bitmap into a 4bpp hardware tile.
 * Lit pixels (bit=1) → palette index 1; off pixels → index 0.
 * tile_idx: absolute tile index in charblock 0 (used for BG0).
 * ---------------------------------------------------------------- */
static void hud_upload_glyph(u16 tile_idx, const u8 bitmap[8])
{
    /* tile4_mem[charblock][tile] addresses 4bpp tiles in VRAM.
     * Each u32 word encodes 8 pixels (4 bits each, low nibble = left pixel). */
    u8 row;
    for (row = 0; row < 8; row++) {
        u8  mask = bitmap[row];
        u32 word = 0;
        u8  col;
        for (col = 0; col < 8; col++) {
            if (mask & (0x80u >> col)) {
                word |= (u32)(1u << (col * 4u));
            }
        }
        tile4_mem[TE_CBB_BG0][tile_idx].data[row] = word;
    }
}

/* ----------------------------------------------------------------
 * Write a null-terminated string into BG0 row 0 starting at tile column col.
 * ---------------------------------------------------------------- */
static void hud_write_str(u8 col, const char *s)
{
    while (*s && col < HUD_TILE_COLS) {
        se_mem[TE_SBB_BG0][0][col] = (SCREENENTRY)hud_char_to_tile(*s);
        col++;
        s++;
    }
}

/* ----------------------------------------------------------------
 * Convert a u16 cent value to "$XXX.XX" (7 chars, null-terminated).
 * buf must be at least 8 bytes.  Values up to 9999 cents ($99.99).
 * ---------------------------------------------------------------- */
static void cents_to_dollar_str(u16 cents, char *buf)
{
    u16 dollars = cents / 100u;
    u8  pennies = (u8)(cents % 100u);
    u8  i       = 0;

    buf[i++] = '$';

    if (dollars >= 100u) {
        buf[i++] = (char)('0' + dollars / 100u);
        dollars %= 100u;
        buf[i++] = (char)('0' + dollars / 10u);
        buf[i++] = (char)('0' + dollars % 10u);
    } else if (dollars >= 10u) {
        buf[i++] = (char)('0' + dollars / 10u);
        buf[i++] = (char)('0' + dollars % 10u);
    } else {
        buf[i++] = (char)('0' + dollars);
    }

    buf[i++] = '.';
    buf[i++] = (char)('0' + pennies / 10u);
    buf[i++] = (char)('0' + pennies % 10u);
    buf[i]   = '\0';
}

/* ----------------------------------------------------------------
 * hud_init
 * ---------------------------------------------------------------- */
void hud_init(void)
{
    u16 g;
    u16 col;

    /* Upload all font glyphs into charblock 0 starting at tile HUD_GLYPH_BASE */
    for (g = 0; g < HUD_NUM_GLYPHS; g++) {
        hud_upload_glyph((u16)(HUD_GLYPH_BASE + g), k_font_bitmaps[g]);
    }

    /* Fill top row of BG0 screenblock with a dark bar (space glyph = solid dark) */
    for (col = 0; col < HUD_TILE_COLS; col++) {
        se_mem[TE_SBB_BG0][0][col] = (SCREENENTRY)HUD_GLYPH_SPACE;
    }

    /* BG0 scroll is locked in tile_engine_init(); do not change it here */
}

/* ----------------------------------------------------------------
 * hud_draw — called every frame; updates all four zones
 *
 * Zone 1 cols  0– 8: "$XXX.XX" cash balance
 * Zone 2 cols  9–15: "BOX:XX"  backroom total
 * Zone 3 cols 16–21: "DAY X"   current day
 * Zone 4 cols 22–29: "HH:MM AP" time string
 * ---------------------------------------------------------------- */
void hud_draw(void)
{
    char buf[12];
    u8   n;

    /* Zone 1: cash */
    cents_to_dollar_str(g_cash, buf);
    hud_write_str(0, buf);
    /* pad to 9 columns */
    for (n = 0; buf[n]; n++) {}
    while (n < 9u) { se_mem[TE_SBB_BG0][0][n++] = (SCREENENTRY)HUD_GLYPH_SPACE; }

    /* Zone 2: backroom total */
    buf[0] = 'B'; buf[1] = 'O'; buf[2] = 'X'; buf[3] = ':';
    buf[4] = (char)('0' + g_backroom_total / 10u);
    buf[5] = (char)('0' + g_backroom_total % 10u);
    buf[6] = '\0';
    hud_write_str(9, buf);

    /* Zone 3: day number */
    buf[0] = 'D'; buf[1] = 'A'; buf[2] = 'Y'; buf[3] = ' ';
    buf[4] = (char)('0' + g_daytimer.day % 10u);
    buf[5] = '\0';
    if (g_daytimer.day >= 10u) {
        buf[4] = (char)('0' + g_daytimer.day / 10u);
        buf[5] = (char)('0' + g_daytimer.day % 10u);
        buf[6] = '\0';
    }
    hud_write_str(16, buf);

    /* Zone 4: time string */
    daytimer_get_time_str(buf);
    hud_write_str(22, buf);
}
