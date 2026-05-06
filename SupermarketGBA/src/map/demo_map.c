#include "demo_map.h"
#include "tile_engine.h"
#include "constants.h"

/*
 * Short tile-ID aliases — local to this translation unit only.
 *
 * S1 and S2 share tile ID 40 (stocked).
 * S3 and SH share tile ID 32 (empty shelf / shelf front).
 * DM/DR are door-mat and door tiles at the south exit.
 */
#define FL   0
#define WC  16
#define WT  17
#define WL  18
#define WR  19
#define SH  32   /* shelf front / empty shelf  */
#define S1  40   /* shelf 1 — stocked          */
#define S2  40   /* shelf 2 — stocked          */
#define S3  32   /* shelf 3 — always empty     */
#define CT  48   /* counter top                */
#define RG  49   /* register                   */
#define DM  64   /* door mat                   */
#define DR  65   /* door                       */
#define PO  96   /* potted plant               */

/* ----------------------------------------------------------------
 * BG2 — floor + walls layer
 *
 * Contains structural tiles only (walls, floor, door mat/door).
 * All foreground fixtures are represented as FL here so BG1 shows
 * through on top.
 * ---------------------------------------------------------------- */
const u8 demo_map_bg2[150] = {
    /* row 0 */ WC,WT,WT,WT,WT,WT,WT,WT,WT,WT,WT,WT,WT,WT,WC,
    /* row 1 */ WL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,WR,
    /* row 2 */ WL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,WR,
    /* row 3 */ WL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,WR,
    /* row 4 */ WL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,WR,
    /* row 5 */ WL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,WR,
    /* row 6 */ WL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,WR,
    /* row 7 */ WL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,WR,
    /* row 8 */ WL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,WR,
    /* row 9 */ WC,FL,FL,FL,FL,FL,DM,DR,DR,DM,FL,FL,FL,FL,WC,
};

/* ----------------------------------------------------------------
 * BG1 — shelf fronts and counter layer (rendered in front of BG2)
 *
 * FL (tile 0) acts as transparent where no foreground element exists.
 * Row 1: shelf tops (SH) and potted plants (PO) facing the aisles.
 * Row 2: stocked shelves S1 (cols 3-5) and S2 (cols 8-10).
 * Rows 4-5: second shelf group — front (SH) then empty stock (S3).
 * Row 7: counter tops (CT) spanning cols 7-11.
 * Row 8: register (RG) at col 7.
 * ---------------------------------------------------------------- */
const u8 demo_map_bg1[150] = {
    /* row 0 */ FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,
    /* row 1 */ FL,PO,PO,SH,SH,SH,FL,FL,SH,SH,SH,PO,PO,PO,FL,
    /* row 2 */ FL,FL,FL,S1,S1,S1,FL,FL,S2,S2,S2,FL,FL,FL,FL,
    /* row 3 */ FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,
    /* row 4 */ FL,FL,FL,SH,SH,SH,FL,FL,FL,FL,FL,FL,FL,FL,FL,
    /* row 5 */ FL,FL,FL,S3,S3,S3,FL,FL,FL,FL,FL,FL,FL,FL,FL,
    /* row 6 */ FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,
    /* row 7 */ FL,FL,FL,FL,FL,FL,FL,CT,CT,CT,CT,CT,FL,FL,FL,
    /* row 8 */ FL,FL,FL,FL,FL,FL,FL,RG,FL,FL,FL,FL,FL,FL,FL,
    /* row 9 */ FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,FL,
};

/* ----------------------------------------------------------------
 * Interaction zones
 *
 * Each entry is the (col, row) of a tile the player faces when
 * standing directly south of the fixture.
 *
 * Shelf 1 (S1 at row 2, cols 3-5): player stands at row 3, cols 3-5.
 * Shelf 2 (S2 at row 2, cols 8-10): player stands at row 3, cols 8-10.
 * Shelf 3 (S3 at row 5, cols 3-5): player stands at row 6, cols 3-5.
 * Register (RG at row 8, col 7): single tile, player faces it directly.
 * ---------------------------------------------------------------- */
const TileCoord SHELF1_TILES[SHELF1_COUNT] = {{3,3},{4,3},{5,3}};
const TileCoord SHELF2_TILES[SHELF2_COUNT] = {{8,3},{9,3},{10,3}};
const TileCoord SHELF3_TILES[SHELF3_COUNT] = {{3,6},{4,6},{5,6}};
const TileCoord REGISTER_TILE              = {7,8};

/* ----------------------------------------------------------------
 * Mutable runtime copy of BG1 (foreground layer).
 * Initialized from demo_map_bg1[] in demo_map_load().
 * map_tile_at() and demo_map_set_shelf_state() operate on this copy.
 * ---------------------------------------------------------------- */
static u8 g_map_bg1_rt[MAP_W * MAP_H];

/* ----------------------------------------------------------------
 * map_tile_at — effective tile ID at (x, y) for collision / interact
 * ---------------------------------------------------------------- */
u8 map_tile_at(u8 x, u8 y)
{
    u8 fg, bg;
    if (x >= MAP_W || y >= MAP_H) return WC;  /* treat OOB as solid wall */
    fg = g_map_bg1_rt[(u16)y * MAP_W + x];
    if (fg != FL) return fg;
    bg = demo_map_bg2[(u16)y * MAP_W + x];
    return bg;
}

/* ----------------------------------------------------------------
 * Dirty flags and pending state for deferred VRAM shelf updates.
 * Bit i of g_shelf_dirty is set when shelf i needs a VRAM flush.
 * g_shelf_stocked[i] holds the target stocked value.
 * ---------------------------------------------------------------- */
static u8 g_shelf_dirty   = 0;
static u8 g_shelf_stocked[3];

/* ----------------------------------------------------------------
 * demo_map_set_shelf_state — update runtime map array and set dirty flag.
 * VRAM write is deferred to demo_map_flush_dirty().
 * ---------------------------------------------------------------- */
void demo_map_set_shelf_state(u8 shelf_id, u8 stocked)
{
    static const u8 shelf_row[3] = { 2, 2, 5 };
    static const u8 shelf_col[3] = { 3, 8, 3 };
    u8 new_tile = stocked ? S1 : SH;
    u8 r = shelf_row[shelf_id];
    u8 c = shelf_col[shelf_id];
    u8 i;

    if (shelf_id >= 3) return;

    /* Update the mutable runtime map (no VRAM write here) */
    for (i = 0; i < 3; i++) {
        g_map_bg1_rt[(u16)r * MAP_W + c + i] = new_tile;
    }

    /* Record pending VRAM update */
    g_shelf_stocked[shelf_id] = stocked;
    g_shelf_dirty |= (u8)(1u << shelf_id);
}

/* ----------------------------------------------------------------
 * demo_map_flush_dirty — write pending shelf tile changes to VRAM.
 * Called once per frame inside the VBlank window.
 * ---------------------------------------------------------------- */
void demo_map_flush_dirty(void)
{
    static const u8 shelf_row[3] = { 2, 2, 5 };
    static const u8 shelf_col[3] = { 3, 8, 3 };
    u8 s;

    if (!g_shelf_dirty) return;

    for (s = 0; s < 3u; s++) {
        if (g_shelf_dirty & (u8)(1u << s)) {
            u8 new_tile = g_shelf_stocked[s] ? S1 : SH;
            u8 r = shelf_row[s];
            u8 c = shelf_col[s];
            u8 i;
            for (i = 0; i < 3u; i++) {
                se_mem[TE_SBB_BG1][r][c + i] = (SCREENENTRY)new_tile;
            }
        }
    }
    g_shelf_dirty = 0;
}

/* ----------------------------------------------------------------
 * demo_map_load
 * ---------------------------------------------------------------- */
void demo_map_load(void) {
    u16 i;

    /* Initialize the mutable BG1 runtime copy */
    for (i = 0; i < MAP_W * MAP_H; i++) {
        g_map_bg1_rt[i] = demo_map_bg1[i];
    }

    /* Collision flags for every tile ID present in this map. */
    g_collision_flags[FL] = 0x00;
    g_collision_flags[WC] = COL_SOLID;
    g_collision_flags[WT] = COL_SOLID;
    g_collision_flags[WL] = COL_SOLID;
    g_collision_flags[WR] = COL_SOLID;
    /* SH (empty shelf face) gets INTERACT so player can stock it.
     * The shelf HEADER (also tile 32, row 1) is unreachable from the player
     * side because the shelf face at row 2 is solid, so no false interact. */
    g_collision_flags[SH] = COL_SOLID|COL_INTERACT|COL_SHELF;  /* covers S3=32 */
    g_collision_flags[S1] = COL_SOLID|COL_INTERACT|COL_SHELF; /* covers S2=40 */
    g_collision_flags[CT] = COL_SOLID;
    g_collision_flags[RG] = COL_SOLID|COL_REGISTER;
    g_collision_flags[DM] = 0x00;
    g_collision_flags[DR] = COL_SPAWN|COL_EXIT;
    g_collision_flags[PO] = COL_SOLID;

    tile_engine_load_map(demo_map_bg2, MAP_W, MAP_H, 2);
    tile_engine_load_map(demo_map_bg1, MAP_W, MAP_H, 1);
}
