#pragma once
#include "sanctum.h"

/* Sanctum 2 — The Tidecrypt (Coastal Shore, W). Drops: Tide Shard + Coralite */
void tidecrypt_init(Sanctum *s);
void tide_update(void);
void drowned_warden_update(Boss *boss, s16 player_x, s16 player_y);
