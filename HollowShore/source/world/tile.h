#pragma once
#include "../core/types.h"

typedef enum {
    TILE_GRASS = 0,
    TILE_DIRT,
    TILE_WATER,
    TILE_TREE,
    TILE_ROCK,
    TILE_TILLED,
    TILE_CROP_1,
    TILE_CROP_2,
    TILE_CROP_3,
    TILE_LAVA,
    TILE_ICE,
    TILE_CORRUPTION,
    TILE_COUNT
} TileType;

typedef enum {
    RESOURCE_NONE = 0,
    RESOURCE_TREE,
    RESOURCE_ROCK,
    RESOURCE_GRASS,
    RESOURCE_WATER
} ResourceType;

typedef struct {
    bool         passable;
    ResourceType resource_type;
} TileDef;

extern const TileDef tile_defs[TILE_COUNT];
