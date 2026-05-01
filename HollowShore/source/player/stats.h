#pragma once
#include "../core/types.h"

typedef enum {
    HUNGER_FULL = 0,
    HUNGER_SATISFIED,
    HUNGER_HUNGRY,
    HUNGER_STARVING
} HungerStage;

typedef struct {
    s16 hp;
    s16 max_hp;
    s16 stamina;
    s16 max_stamina;
    s16 hunger;
    s16 max_hunger;
} PlayerStats;

void       stats_init(PlayerStats *s);
void       stamina_update(PlayerStats *s, bool sprinting, bool using_tool, bool near_bed);
void       hunger_update(PlayerStats *s, bool is_active);
HungerStage hunger_stage(const PlayerStats *s);
