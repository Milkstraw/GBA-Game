#include "tile_engine.h"

/* Collision bitmask table — zero = walkable, non-zero = blocked/special.
 * Populated by game code before any map is loaded. */
u8 g_collision_flags[256];

/* Screenblock index for each layer, matching the header constants. */
static const u8 k_sbb[4] = { TE_SBB_BG0, TE_SBB_BG1, TE_SBB_BG2, TE_SBB_BG3 };

/* ---------------------------------------------------------------------------
 * tile_engine_init
 *
 * REG_DISPCNT  — Mode 0, BG0–BG3 all enabled.
 *
 * BGxCNT layout (all layers):
 *   bits 1–0  : priority  (BG0 highest → BG3 lowest, drawn back-to-front)
 *   bits 3–2  : char base block
 *   bit  7    : 0 = 16-colour / 16-palette (4 bpp)
 *   bits 12–8 : screen base block
 *   bits 15–14: 00 = 256×256 px map (32×32 tiles, 1 screenblock)
 * --------------------------------------------------------------------------- */
void tile_engine_init(void)
{
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_BG3;

    /* BG0 — HUD, topmost layer */
    REG_BG0CNT = BG_PRIO(0) | BG_CBB(TE_CBB_BG0) | BG_SBB(TE_SBB_BG0)
               | BG_4BPP  | BG_REG_32x32;

    /* BG1 — foreground (entities, items) */
    REG_BG1CNT = BG_PRIO(1) | BG_CBB(TE_CBB_BG1) | BG_SBB(TE_SBB_BG1)
               | BG_4BPP  | BG_REG_32x32;

    /* BG2 — floor and walls */
    REG_BG2CNT = BG_PRIO(2) | BG_CBB(TE_CBB_BG2) | BG_SBB(TE_SBB_BG2)
               | BG_4BPP  | BG_REG_32x32;

    /* BG3 — background decoration, drawn first (behind everything) */
    REG_BG3CNT = BG_PRIO(3) | BG_CBB(TE_CBB_BG3) | BG_SBB(TE_SBB_BG3)
               | BG_4BPP  | BG_REG_32x32;
}

/* ---------------------------------------------------------------------------
 * tile_engine_load_map
 *
 * Writes map_data[row*w + col] as a SCREENENTRY (u16) into the correct
 * screenblock.  w and h are clamped to 32 — the hardware limit for a
 * 256×256 (32×32 tile) screenblock in Mode 0.
 *
 * Tile IDs arrive as u8 and are stored as-is in bits 9–0 of the entry
 * (palette 0, no flip).  The caller can OR in flip/palette bits afterwards
 * if needed by writing directly to se_mem[sbb][row][col].
 * --------------------------------------------------------------------------- */
void tile_engine_load_map(const u8 *map_data, u16 w, u16 h, u8 bg_layer)
{
    u16 sbb = k_sbb[bg_layer & 3u];
    u16 rows = (h < 32u) ? h : 32u;
    u16 cols = (w < 32u) ? w : 32u;

    for (u16 row = 0; row < rows; row++) {
        for (u16 col = 0; col < cols; col++) {
            se_mem[sbb][row][col] = (SCREENENTRY)map_data[row * w + col];
        }
    }
}

/* ---------------------------------------------------------------------------
 * tile_engine_set_scroll
 *
 * BGxHOFS / BGxVOFS are write-only 9-bit registers; casting s16 → u16
 * preserves the two's-complement bit pattern the hardware expects for
 * negative (leftward / upward) scroll.
 * --------------------------------------------------------------------------- */
void tile_engine_set_scroll(u8 bg, s16 dx, s16 dy)
{
    switch (bg & 3u) {
    case 0:
        REG_BG0HOFS = (u16)dx;
        REG_BG0VOFS = (u16)dy;
        break;
    case 1:
        REG_BG1HOFS = (u16)dx;
        REG_BG1VOFS = (u16)dy;
        break;
    case 2:
        REG_BG2HOFS = (u16)dx;
        REG_BG2VOFS = (u16)dy;
        break;
    case 3:
        REG_BG3HOFS = (u16)dx;
        REG_BG3VOFS = (u16)dy;
        break;
    }
}
