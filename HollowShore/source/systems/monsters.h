#pragma once
#include "../core/types.h"

#define MAX_ENEMIES  10

typedef enum {
    ENEMY_SLIME = 0,
    ENEMY_BAT,
    ENEMY_WOLF,
    ENEMY_GOBLIN,
    ENEMY_GOBLIN_ARMORED,
    ENEMY_WRAITH,
    ENEMY_BEAR,
    ENEMY_SWIMMER,
    ENEMY_GOLEM,
    ENEMY_VINE_WHIP,
    ENEMY_TIDE_CRAWLER,
    ENEMY_MIMIC,
    ENEMY_ECHO,
    ENEMY_COUNT
} EnemyType;

typedef enum {
    ESTATE_IDLE = 0,
    ESTATE_CHASE,
    ESTATE_ATTACK,
    ESTATE_DEAD
} EnemyState;

typedef struct {
    EnemyType  type;
    s16        x, y;
    s16        hp;
    EnemyState state;
    bool       active;
    u8         sprite_id;
} Enemy;

extern Enemy enemies[MAX_ENEMIES];

void  enemy_init_pool(void);
void  enemy_spawn(EnemyType type, s16 x, s16 y);
void  enemy_update_all(s16 player_x, s16 player_y, u16 day, bool is_night);
void  enemy_take_hit(u8 index, u8 damage);
u8    spawn_tier_from_day(u16 day);
void  siege_event_check(u16 day, u16 night_count);
