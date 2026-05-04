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

#endif /* DEMO_MAP_H */
