#pragma once
#include "sanctum.h"
#include "bosses.h"
#include "../systems/monsters.h"

/* Sanctum 4 — The Marshveil (Marshland, E). Drops: Veil Shard + Spectral Dust */
void marshveil_init(Sanctum *s);
void fog_window_update(u8 torch_x, u8 torch_y);
void mimic_enemy_update(Enemy *e, s16 player_x, s16 player_y);
void bog_specter_update(Boss *boss, s16 player_x, s16 player_y);
