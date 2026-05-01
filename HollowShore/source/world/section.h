#pragma once
#include "../core/types.h"
#include "tile.h"

#define SECTION_TILES_W  160
#define SECTION_TILES_H  160
#define TILE_SIZE        16
#define SECTION_COUNT    9   /* 3×3 grid */

typedef struct {
    u8   tile_id;
    u8   respawn_day;   /* 0 = no respawn pending */
} ResourceNode;

typedef struct {
    u8           id;                               /* 1–9, row-major */
    u8           tiles[SECTION_TILES_W * SECTION_TILES_H];
    ResourceNode resources[SECTION_TILES_W * SECTION_TILES_H];
    s16          cam_x;
    s16          cam_y;
} Section;

void section_load(u8 id, Section *out);
void section_save_state(const Section *s);
void section_set_tile(Section *s, u8 x, u8 y, TileType type);
TileType section_get_tile(const Section *s, u8 x, u8 y);
u8   section_id_from_direction(u8 current_id, u8 dir);
