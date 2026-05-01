#pragma once
#include "sanctum.h"

/* Sanctum 3 — The Frostspire (Tundra, NW). Drops: Frost Shard + Glacite Ore */
void frostspire_init(Sanctum *s);
void ice_momentum(s16 *vx, s16 *vy, u8 facing);
void glacial_warden_update(Boss *boss, s16 player_x, s16 player_y);
