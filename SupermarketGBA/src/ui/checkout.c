#include <tonc.h>
#include "checkout.h"
#include "npc.h"
#include "inventory.h"
#include "input.h"
#include "tile_engine.h"
#include "hud.h"

/* ----------------------------------------------------------------
 * Local helpers: reuse the same glyph-tile mapping as inventory_screen
 * ---------------------------------------------------------------- */
static u16 co_char_to_tile(char c)
{
    if (c == ' ')              return HUD_GLYPH_SPACE;
    if (c >= '0' && c <= '9') return (u16)(HUD_GLYPH_DIGIT0 + (c - '0'));
    if (c >= 'A' && c <= 'Z') return (u16)(HUD_GLYPH_A + (c - 'A'));
    if (c >= 'a' && c <= 'z') return (u16)(HUD_GLYPH_A + (c - 'a'));
    if (c == '$') return HUD_GLYPH_DOLLAR;
    if (c == '.') return HUD_GLYPH_DOT;
    if (c == '+') return HUD_GLYPH_PLUS;
    if (c == ':') return HUD_GLYPH_COLON;
    return HUD_GLYPH_SPACE;
}

static void co_write(u8 col, u8 row, const char *s)
{
    while (*s && col < 30u) {
        se_mem[TE_SBB_BG1][row][col] = (SCREENENTRY)co_char_to_tile(*s);
        col++;
        s++;
    }
}

static void co_write_cents(u8 col, u8 row, u16 cents)
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
    co_write(col, row, buf);
}

static void co_clear_row(u8 row)
{
    u8 col;
    for (col = 0; col < 30u; col++) {
        se_mem[TE_SBB_BG1][row][col] = (SCREENENTRY)HUD_GLYPH_SPACE;
    }
}

/* Save/restore 10 rows of BG1 */
static u16 s_co_bg1_backup[10][30];

static void co_save_bg1(void)
{
    u8 r, c;
    for (r = 0; r < 10u; r++)
        for (c = 0; c < 30u; c++)
            s_co_bg1_backup[r][c] = se_mem[TE_SBB_BG1][r][c];
}

static void co_restore_bg1(void)
{
    u8 r, c;
    for (r = 0; r < 10u; r++)
        for (c = 0; c < 30u; c++)
            se_mem[TE_SBB_BG1][r][c] = s_co_bg1_backup[r][c];
}

/* ----------------------------------------------------------------
 * Float-sprite animation: "+$X.XX" rises 1px per 2 frames for 60 frames.
 * Uses OAM slot 5 (temporary).
 * ---------------------------------------------------------------- */
static void co_float_animate(u16 amount_cents)
{
    char  buf[8];
    u8    i = 0;
    u16   d = amount_cents / 100u;
    u8    p = (u8)(amount_cents % 100u);
    u8    frame;

    /* Build the amount string in the first 3 tiles of a scratch tile (unused) */
    buf[i++] = '+';
    buf[i++] = '$';
    if (d >= 10u) buf[i++] = (char)('0' + d / 10u);
    buf[i++] = (char)('0' + d % 10u);
    buf[i++] = '.';
    buf[i++] = (char)('0' + p / 10u);
    buf[i++] = (char)('0' + p % 10u);
    buf[i]   = '\0';

    /* Show the string as BG1 text so we don't need a real sprite tile.
     * Place it at row 5, centered around col 12, and scroll it up. */
    (void)buf;  /* suppress unused warning — full sprite approach below */

    /* Write "+$X.XX" to BG1 row 5 as a static line for 60 frames.
     * A full OAM float sprite requires a loaded sprite sheet — use text overlay. */
    co_write(10, 5, "+");
    co_write_cents(11, 5, amount_cents);

    for (frame = 0; frame < 60u; frame++) {
        VBlankIntrWait();
        input_poll();  /* drain input during animation */
    }

    /* Clear the float line */
    co_clear_row(5);
}

/* ----------------------------------------------------------------
 * checkout_start
 * ---------------------------------------------------------------- */
void checkout_start(void)
{
    NPC  *n = g_npc_at_register;
    u16   total;

    /* Guard: no customer waiting */
    if (!g_customer_waiting || !n) {
        /* Show "NO CUSTOMER" for 30 frames using BG1 overlay */
        co_save_bg1();
        co_clear_row(5);
        co_write(7, 5, "NO CUSTOMER");
        {
            u8 t;
            for (t = 0; t < 30u; t++) { VBlankIntrWait(); input_poll(); }
        }
        co_restore_bg1();
        return;
    }

    /* --- Checkout overlay --- */
    co_save_bg1();
    {
        u8 r;
        for (r = 0; r < 10u; r++) co_clear_row(r);
    }

    co_write(8, 0, "CHECKOUT");

    /* List items purchased */
    {
        u8 row = 2;
        u8 i;
        for (i = 0; i < n->list_size && i < n->cart_count; i++) {
            u8 pid = n->shopping_list[i];
            if (pid < MAX_PRODUCTS) {
                /* Write product name */
                u8 j, col = 0;
                for (j = 0; j < 12u && g_products[pid].name[j]; j++) {
                    se_mem[TE_SBB_BG1][row][col++] =
                        (SCREENENTRY)co_char_to_tile(g_products[pid].name[j]);
                }
                /* Write " x1  $X.XX" */
                co_write((u8)(col + 1u), row, "X1");
                co_write_cents((u8)(col + 5u), row, g_products[pid].shelf_price);
                row++;
            }
        }
    }

    /* Total */
    total = n->cart_total;
    co_write(0, 7, "TOTAL: ");
    co_write_cents(7, 7, total);

    co_write(0, 9, "A=COLLECT");

    /* Wait for player to press A */
    while (1) {
        VBlankIntrWait();
        input_poll();
        if (KEY_PRESSED(KEY_A)) break;
        if (KEY_PRESSED(KEY_B)) {
            /* Allow cancel */
            co_restore_bg1();
            return;
        }
    }

    /* Process checkout — npc_checkout adds cart_total to g_cash */
    npc_checkout(n);

    /* Show "+$X.XX" float animation */
    co_float_animate(total);

    co_restore_bg1();
}
