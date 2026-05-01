#pragma once
#include "../core/types.h"

typedef enum {
    BOSS_ROOT_WARDEN = 0,
    BOSS_DROWNED_WARDEN,
    BOSS_GLACIAL_WARDEN,
    BOSS_BOG_SPECTER,
    BOSS_EMBER_TITAN,
    BOSS_HOLLOW_SOVEREIGN
} BossType;

typedef struct {
    BossType type;
    s16      x, y;
    s16      hp;
    s16      max_hp;
    u8       phase;
    bool     active;
    u8       sprite_id;
} Boss;

extern Boss active_boss;

void boss_take_hit(u8 damage);
bool boss_is_defeated(void);
