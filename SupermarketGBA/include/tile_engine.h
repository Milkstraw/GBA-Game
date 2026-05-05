#ifndef TILE_ENGINE_H
#define TILE_ENGINE_H

#include <tonc.h>

/*
 * VRAM layout (Mode 0, 4 BG layers, 16-color tiles, 256×256 maps)
 *
 *   BG  | role           | charblock | screenblock
 *   ----+----------------+-----------+------------
 *    0  | HUD (fixed)    |     0     |     7
 *    1  | foreground     |     1     |    15
 *    2  | floor / walls  |     2     |    23
 *    3  | background deco|     3     |    31
 *
 * Each charblock owns SBBs [n*8 .. n*8+7].  Tile data fills the lower
 * SBBs; the map sits at the last SBB so 14 KB remain for tile graphics.
 */

/* Screenblock index for each BG layer (read-only, used by map loader). */
#define TE_SBB_BG0   7
#define TE_SBB_BG1  15
#define TE_SBB_BG2  23
#define TE_SBB_BG3  31

/* Charblock (character base block) for each BG layer. */
#define TE_CBB_BG0   0
#define TE_CBB_BG1   1
#define TE_CBB_BG2   2
#define TE_CBB_BG3   3

/* ------------------------------------------------------------
 * Collision bitmask flags stored in g_collision_flags[tile_id]
 * ------------------------------------------------------------ */
#define COL_SOLID     (1u << 0)   /* impassable wall / obstacle  */
#define COL_INTERACT  (1u << 1)   /* player can press A to use   */
#define COL_SHELF     (1u << 2)   /* shelf face tile              */
#define COL_REGISTER  (1u << 3)   /* checkout register tile       */
#define COL_SPAWN     (1u << 4)   /* NPC / player spawn point     */
#define COL_EXIT      (1u << 5)   /* room exit / door tile        */
#define COL_BACKROOM  (1u << 6)   /* backroom-only zone           */

/*
 * Collision table — one byte per tile ID (0–255).
 * Zero-initialised at startup; populate before loading any map.
 */
extern u8 g_collision_flags[256];

/* ------------------------------------------------------------
 * API
 * ------------------------------------------------------------ */

/*
 * Configure REG_DISPCNT (Mode 0, all 4 BGs enabled) and all four
 * BGxCNT registers (priority, charblock, screenblock, 16-colour,
 * 256×256 map).  Call once before the main loop.
 */
void tile_engine_init(void);

/*
 * Write a flat row-major tile-ID array into the screenblock of the
 * given BG layer.  map_data[row*w + col] → screenblock entry.
 * w and h are clamped to 32 (the hardware screenblock dimension).
 * bg_layer: 0–3.
 */
void tile_engine_load_map(const u8 *map_data, u16 w, u16 h, u8 bg_layer);

/*
 * Set the horizontal and vertical scroll offsets for a BG layer.
 * Writes directly to BGxHOFS / BGxVOFS.  bg: 1–3.
 * BG0 (HUD) is ignored — scroll is locked to (0,0) by tile_engine_init.
 */
void tile_engine_set_scroll(u8 bg, s16 dx, s16 dy);

#endif /* TILE_ENGINE_H */
