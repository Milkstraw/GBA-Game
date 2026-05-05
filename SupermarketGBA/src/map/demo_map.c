#include "demo_map.h"
#include "tile_engine.h"

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
 * demo_map_load
 * ---------------------------------------------------------------- */
void demo_map_load(void) {
    /* Collision flags for every tile ID present in this map. */
    g_collision_flags[FL] = 0x00;
    g_collision_flags[WC] = COL_SOLID;
    g_collision_flags[WT] = COL_SOLID;
    g_collision_flags[WL] = COL_SOLID;
    g_collision_flags[WR] = COL_SOLID;
    g_collision_flags[SH] = COL_SOLID;                         /* covers S3=32 */
    g_collision_flags[S1] = COL_SOLID|COL_INTERACT|COL_SHELF; /* covers S2=40 */
    g_collision_flags[CT] = COL_SOLID;
    g_collision_flags[RG] = COL_SOLID|COL_REGISTER;
    g_collision_flags[DM] = 0x00;
    g_collision_flags[DR] = COL_SPAWN|COL_EXIT;
    g_collision_flags[PO] = COL_SOLID;

    tile_engine_load_map(demo_map_bg2, MAP_W, MAP_H, 2);
    tile_engine_load_map(demo_map_bg1, MAP_W, MAP_H, 1);
}
