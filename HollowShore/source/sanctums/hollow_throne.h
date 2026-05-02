#pragma once
#include "sanctum.h"
#include "bosses.h"
#include "../systems/monsters.h"
#include "../player/inventory.h"

/* Sanctum 6 — The Hollow Throne (Corrupted, SE). Drops: Hollow Keystone (Portal Piece 6) */
void hollow_throne_init(Sanctum *s);
bool entry_gate_check(const Inventory *inv);   /* requires all 5 prior shards */
void corruption_tile_drain(s16 player_x, s16 player_y);
void echo_enemy_update(Enemy *e, s16 player_x, s16 player_y);
void hollow_sovereign_update(Boss *boss, s16 player_x, s16 player_y, u16 *keys);
