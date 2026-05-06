#include <tonc.h>
#include "inventory_screen.h"
#include "inventory.h"
#include "input.h"
#include "tile_engine.h"
#include "hud.h"
#include "daytimer.h"

/* Number of demo products shown in inventory */
#define INV_NUM_PRODUCTS  3u

/* Map an ASCII character to a HUD glyph tile index (from hud.h) */
static u16 inv_char_to_tile(char c)
{
    if (c == ' ')              return HUD_GLYPH_SPACE;
    if (c >= '0' && c <= '9') return (u16)(HUD_GLYPH_DIGIT0 + (c - '0'));
    if (c >= 'A' && c <= 'Z') return (u16)(HUD_GLYPH_A + (c - 'A'));
    if (c >= 'a' && c <= 'z') return (u16)(HUD_GLYPH_A + (c - 'a'));
    if (c == '$') return HUD_GLYPH_DOLLAR;
    if (c == '.') return HUD_GLYPH_DOT;
    if (c == ':') return HUD_GLYPH_COLON;
    if (c == '>') return HUD_GLYPH_PLUS;   /* reuse + for cursor > */
    if (c == '<') return HUD_GLYPH_MINUS;  /* reuse - for < */
    return HUD_GLYPH_SPACE;
}

/* Write a string to BG1 screenblock at (col, row) */
static void inv_write(u8 col, u8 row, const char *s)
{
    while (*s && col < 30u) {
        se_mem[TE_SBB_BG1][row][col] = (SCREENENTRY)inv_char_to_tile(*s);
        col++;
        s++;
    }
}

/* Write a u8 as 2 decimal digits at (col, row) */
static void inv_write_u8(u8 col, u8 row, u8 val)
{
    char buf[3];
    buf[0] = (char)('0' + val / 10u);
    buf[1] = (char)('0' + val % 10u);
    buf[2] = '\0';
    inv_write(col, row, buf);
}

/* Write "$X.XX" into BG1 at (col, row) from a u16 cent value */
static void inv_write_cents(u8 col, u8 row, u16 cents)
{
    char buf[8];
    u8   i = 0;
    u16  d = cents / 100u;
    u8   p = (u8)(cents % 100u);

    buf[i++] = '$';
    if (d >= 10u) buf[i++] = (char)('0' + d / 10u);
    buf[i++] = (char)('0' + d % 10u);
    buf[i++] = '.';
    buf[i++] = (char)('0' + p / 10u);
    buf[i++] = (char)('0' + p % 10u);
    buf[i]   = '\0';
    inv_write(col, row, buf);
}

/* Clear a row of BG1 screenblock to space glyphs */
static void inv_clear_row(u8 row)
{
    u8 col;
    for (col = 0; col < 30u; col++) {
        se_mem[TE_SBB_BG1][row][col] = (SCREENENTRY)HUD_GLYPH_SPACE;
    }
}

/* Save and restore BG1 screenblock to/from a buffer */
static u16 s_bg1_backup[10][30];

static void inv_save_bg1(void)
{
    u8 r, c;
    for (r = 0; r < 10u; r++) {
        for (c = 0; c < 30u; c++) {
            s_bg1_backup[r][c] = se_mem[TE_SBB_BG1][r][c];
        }
    }
}

static void inv_restore_bg1(void)
{
    u8 r, c;
    for (r = 0; r < 10u; r++) {
        for (c = 0; c < 30u; c++) {
            se_mem[TE_SBB_BG1][r][c] = s_bg1_backup[r][c];
        }
    }
}

/* ----------------------------------------------------------------
 * Draw the full inventory screen
 * cursor_row: 0-2 (which product is selected)
 * order_qty[]: order quantity for each product slot
 * feedback_timer: counts down; >0 shows feedback message
 * feedback_ok: 1=ordered, 0=no cash
 * ---------------------------------------------------------------- */
static void inv_draw(u8 cursor_row, u8 order_qty[INV_NUM_PRODUCTS],
                     u8 feedback_timer, u8 feedback_ok)
{
    u8 i;
    char buf[14];

    /* Row 0: title */
    inv_write(6, 0, "INVENTORY");

    /* Row 2: column headers */
    inv_write(0, 2, "PRODUCT  SHELF BACK  COST  ORDER");

    /* Rows 3-5: one row per product */
    for (i = 0; i < INV_NUM_PRODUCTS; i++) {
        u8        r = (u8)(3u + i);
        Product  *p = &g_products[i];
        u8        col = 0;

        inv_clear_row(r);

        /* Cursor arrow */
        if (i == cursor_row) {
            se_mem[TE_SBB_BG1][r][col] = (SCREENENTRY)HUD_GLYPH_PLUS; /* > */
        }
        col = 1;

        /* Product name (up to 8 chars) */
        {
            u8 j;
            for (j = 0; j < 8u && p->name[j]; j++) {
                se_mem[TE_SBB_BG1][r][(u8)(col + j)] =
                    (SCREENENTRY)inv_char_to_tile(p->name[j]);
            }
        }
        col = 9;

        /* Shelf stock */
        inv_write_u8(col, r, p->stock_on_shelf);
        col = 15;

        /* Back stock */
        inv_write_u8(col, r, p->stock_in_back);
        col = 21;

        /* Cost per unit */
        inv_write_cents(col, r, p->cost_price);
        col = 27;

        /* Order quantity selector: ">QQ<" */
        buf[0] = (i == cursor_row) ? '>' : ' ';
        buf[1] = (char)('0' + order_qty[i] / 10u);
        buf[2] = (char)('0' + order_qty[i] % 10u);
        buf[3] = (i == cursor_row) ? '<' : ' ';
        buf[4] = '\0';
        inv_write(col, r, buf);
    }

    /* Row 7: cash */
    inv_clear_row(7);
    inv_write(0, 7, "CASH: ");
    inv_write_cents(6, 7, g_cash);

    /* Row 8: controls hint */
    inv_write(0, 8, "A=ORDER B=CLOSE UD=SEL LR=QTY");

    /* Row 9: feedback */
    inv_clear_row(9);
    if (feedback_timer > 0) {
        inv_write(0, 9, feedback_ok ? "ORDERED OK" : "NO CASH");
    }
}

/* ----------------------------------------------------------------
 * inventory_screen_open — blocking loop until B pressed
 * ---------------------------------------------------------------- */
void inventory_screen_open(void)
{
    u8 cursor    = 0;
    u8 order_qty[INV_NUM_PRODUCTS];
    u8 feedback_timer = 0;
    u8 feedback_ok    = 0;
    u8 i;

    for (i = 0; i < INV_NUM_PRODUCTS; i++) order_qty[i] = 0;

    /* Save BG1 so we can restore it on close */
    inv_save_bg1();

    /* Clear all 10 visible rows on BG1 */
    for (i = 0; i < 10u; i++) inv_clear_row(i);

    /* Main loop — stays here until B is pressed */
    while (1) {
        VBlankIntrWait();
        input_poll();

        /* Close on B */
        if (KEY_PRESSED(KEY_B)) break;

        /* Navigate product rows */
        if (KEY_PRESSED(KEY_UP)) {
            if (cursor > 0) cursor--;
        }
        if (KEY_PRESSED(KEY_DOWN)) {
            if (cursor < INV_NUM_PRODUCTS - 1u) cursor++;
        }

        /* Adjust order quantity (clamp 0-12) */
        if (KEY_PRESSED(KEY_LEFT)) {
            if (order_qty[cursor] > 0) order_qty[cursor]--;
        }
        if (KEY_PRESSED(KEY_RIGHT)) {
            if (order_qty[cursor] < 12u) order_qty[cursor]++;
        }

        /* A: place order */
        if (KEY_PRESSED(KEY_A)) {
            if (order_qty[cursor] > 0) {
                u8 ok = inventory_order((u8)cursor, order_qty[cursor]);
                feedback_ok    = ok;
                feedback_timer = 60u;
                if (ok) order_qty[cursor] = 0;  /* reset qty after success */
            }
        }

        /* Decrement feedback timer */
        if (feedback_timer > 0) feedback_timer--;

        /* Redraw */
        inv_draw(cursor, order_qty, feedback_timer, feedback_ok);
    }

    /* Restore BG1 to game map tiles */
    inv_restore_bg1();
}
