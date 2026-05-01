#pragma once
#include "sanctum.h"

/* Sanctum 5 — The Emberdepth (Volcanic, NE). Drops: Ember Shard + Magma Core */
void emberdepth_init(Sanctum *s);
void lava_tile_damage(s16 player_x, s16 player_y);
void lava_cool(u8 x, u8 y);
void golem_enemy_update(Enemy *e, s16 player_x, s16 player_y);
void ember_titan_update(Boss *boss, s16 player_x, s16 player_y);
