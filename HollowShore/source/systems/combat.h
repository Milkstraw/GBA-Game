#pragma once
#include "../core/types.h"

typedef struct {
    s16  x, y;
    s16  dx, dy;
    u8   damage;
    bool active;
} Projectile;

#define MAX_PROJECTILES  8

extern Projectile projectiles[MAX_PROJECTILES];

void attack_melee(s16 player_x, s16 player_y, u8 facing);
void attack_ranged(s16 player_x, s16 player_y, u8 facing);
void projectile_update(void);
