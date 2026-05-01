#pragma once
#include "../core/types.h"

#define DUNGEON_FLOOR_W   30
#define DUNGEON_FLOOR_H   20
#define DUNGEON_FLOORS     3
#define SANCTUM_COUNT      6

typedef struct {
    u8 tiles[DUNGEON_FLOOR_W * DUNGEON_FLOOR_H];
} DungeonFloor;

typedef struct {
    u8           id;               /* 1–6 */
    DungeonFloor floors[DUNGEON_FLOORS];
    u8           current_floor;    /* 0–2 */
    bool         boss_defeated;
} Sanctum;

void sanctum_enter(u8 sanctum_id);
void sanctum_floor_load(Sanctum *s, u8 floor_index);
void sanctum_exit(Sanctum *s);
