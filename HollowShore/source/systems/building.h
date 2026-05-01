#pragma once
#include "../core/types.h"

#define MAX_STRUCTURES_PER_SECTION  256

typedef enum {
    STRUCT_NONE = 0,
    STRUCT_WALL_WOOD,
    STRUCT_WALL_STONE,
    STRUCT_WALL_BRICK,
    STRUCT_WALL_METAL,
    STRUCT_TORCH,
    STRUCT_LANTERN,
    STRUCT_BRAZIER,
    STRUCT_CAMPFIRE,
    STRUCT_BED,
    STRUCT_WORKBENCH,
    STRUCT_FORGE,
    STRUCT_ALCHEMY_TABLE,
    STRUCT_FENCE,
    STRUCT_DOOR,
    STRUCT_COUNT
} StructureType;

typedef struct {
    StructureType type;
    u8            tier;
    u8            x;
    u8            y;
    u8            hp;
} Structure;

extern Structure placed_structures[MAX_STRUCTURES_PER_SECTION];
extern u8        structure_count;

void building_place(u8 x, u8 y, StructureType type);
void building_break(u8 x, u8 y);
void building_upgrade(u8 x, u8 y);
u8   lighting_radius(StructureType type);
bool tile_in_light(u8 x, u8 y);
