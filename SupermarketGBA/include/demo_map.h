#ifndef DEMO_MAP_H
#define DEMO_MAP_H

#include "gba_types.h"
#include "constants.h"

/* Tile-coordinate pair used for interaction zones. */
typedef struct {
    u8 x; /* column (0-based) */
    u8 y; /* row    (0-based) */
} TileCoord;

/* Tile data — MAP_W * MAP_H = 150 entries each, row-major. */
extern const u8 demo_map_bg2[150]; /* floor + walls          (BG2) */
extern const u8 demo_map_bg1[150]; /* shelf fronts + counter (BG1) */

/*
 * Interaction zones: tile coords the player faces to trigger each fixture.
 * SHELF1/2/3 hold 3 tiles each (the stocked-shelf face row).
 * REGISTER_TILE is the single register tile.
 */
#define SHELF1_COUNT 3
#define SHELF2_COUNT 3
#define SHELF3_COUNT 3

extern const TileCoord SHELF1_TILES[SHELF1_COUNT];
extern const TileCoord SHELF2_TILES[SHELF2_COUNT];
extern const TileCoord SHELF3_TILES[SHELF3_COUNT];
extern const TileCoord REGISTER_TILE;

/*
 * Populate g_collision_flags[] for every tile ID used in the demo map,
 * then call tile_engine_load_map for BG2 (floor/walls) and BG1 (foreground).
 * Must be called after tile_engine_init().
 */
void demo_map_load(void);

/*
 * Return the effective tile ID at tile position (x, y) for collision and
 * interact checks.  BG1 (foreground: shelves, counter, register) takes
 * priority over BG2 (floor/walls) when the BG1 entry is non-zero.
 * Returns a solid tile ID (WC=16) for out-of-bounds coordinates.
 */
u8 map_tile_at(u8 x, u8 y);

/*
 * Mark a shelf's stock state as changed.  Updates the runtime map array and
 * sets a dirty flag; the VRAM write is deferred to demo_map_flush_dirty()
 * which must be called once at the start of the draw phase (inside VBlank).
 * shelf_id: 0=S1, 1=S2, 2=S3.  stocked: 1=stocked (tile 40), 0=empty (tile 32).
 */
void demo_map_set_shelf_state(u8 shelf_id, u8 stocked);

/*
 * Flush any pending shelf-tile changes to VRAM.  Call once per frame,
 * inside the VBlank window (right after VBlankIntrWait).
 */
void demo_map_flush_dirty(void);

#endif /* DEMO_MAP_H */
