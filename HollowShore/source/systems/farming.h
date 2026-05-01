#pragma once
#include "../core/types.h"

typedef enum {
    SEASON_SPRING = 0,
    SEASON_SUMMER,
    SEASON_AUTUMN,
    SEASON_WINTER
} Season;

typedef enum {
    SEED_BASIC = 0,
    SEED_ROOT,
    SEED_GRAIN,
    SEED_HERB,
    SEED_MUSHROOM,
    SEED_COUNT
} SeedType;

typedef struct {
    SeedType seed_type;
    u16      water_day;      /* last day watered */
    u8       growth_stage;   /* 0=planted, 1–3=growing, 4=ready */
} CropTile;

Season season_from_day(u16 day);
void   farming_till(u8 x, u8 y);
void   farming_plant(u8 x, u8 y, SeedType seed);
void   farming_water(u8 x, u8 y, u16 current_day);
void   farming_tick(u16 current_day);
bool   farming_harvest(u8 x, u8 y);
