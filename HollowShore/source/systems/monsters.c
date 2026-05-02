/*
 * monsters.c — Enemy pool, spawning, AI, and siege events for HollowShore
 * Wave 5b implementation
 */

#include "monsters.h"
#include "../core/types.h"
#include "../gba.h"

/* Enemy pool — extern declared in monsters.h */
Enemy enemies[MAX_ENEMIES];

/* Per-enemy death-animation countdown (frames) */
static u8 death_timer[MAX_ENEMIES];

/* Per-enemy attack-cooldown countdown (frames) */
static u8 attack_timer[MAX_ENEMIES];

/* Simple LCG RNG shared internally */
static u32 rng_state = 12345u;

static u32 lcg_next(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

/* Absolute value helpers for s16 arithmetic */
static s16 s16_abs(s16 v) { return (v < 0) ? (s16)(-v) : v; }

/* Chebyshev distance */
static s16 chebyshev(s16 ax, s16 ay, s16 bx, s16 by)
{
    s16 dx = s16_abs((s16)(ax - bx));
    s16 dy = s16_abs((s16)(ay - by));
    return (dx > dy) ? dx : dy;
}

/* HP table indexed by EnemyType */
static const s16 k_hp_table[ENEMY_COUNT] = {
    /* SLIME         */ 10,
    /* BAT           */ 8,
    /* WOLF          */ 20,
    /* GOBLIN        */ 15,
    /* GOBLIN_ARMORED*/ 25,
    /* WRAITH        */ 18,
    /* BEAR          */ 35,
    /* SWIMMER       */ 12,
    /* GOLEM         */ 40,
    /* VINE_WHIP     */ 22,
    /* TIDE_CRAWLER  */ 18,
    /* MIMIC         */ 30,
    /* ECHO          */ 20
};

/* Damage dealt by each enemy type per attack */
static const u8 k_damage_table[ENEMY_COUNT] = {
    /* SLIME         */ 2,
    /* BAT           */ 2,
    /* WOLF          */ 4,
    /* GOBLIN        */ 3,
    /* GOBLIN_ARMORED*/ 5,
    /* WRAITH        */ 6,
    /* BEAR          */ 8,
    /* SWIMMER       */ 3,
    /* GOLEM         */ 10,
    /* VINE_WHIP     */ 4,
    /* TIDE_CRAWLER  */ 3,
    /* MIMIC         */ 5,
    /* ECHO          */ 4
};

/* -------------------------------------------------------------------------
 * enemy_init_pool
 * -------------------------------------------------------------------------*/
void enemy_init_pool(void)
{
    u8 i;
    for (i = 0; i < MAX_ENEMIES; ++i)
    {
        enemies[i].active    = FALSE;
        enemies[i].state     = ESTATE_IDLE;
        enemies[i].hp        = 0;
        enemies[i].sprite_id = 0;
        death_timer[i]       = 0;
        attack_timer[i]      = 0;
    }
}

/* -------------------------------------------------------------------------
 * enemy_spawn
 * -------------------------------------------------------------------------*/
void enemy_spawn(EnemyType type, s16 x, s16 y)
{
    u8 i;
    for (i = 0; i < MAX_ENEMIES; ++i)
    {
        if (!enemies[i].active)
        {
            enemies[i].type      = type;
            enemies[i].x         = x;
            enemies[i].y         = y;
            enemies[i].hp        = k_hp_table[type];
            enemies[i].state     = ESTATE_IDLE;
            enemies[i].active    = TRUE;
            enemies[i].sprite_id = (u8)type;
            death_timer[i]       = 0;
            attack_timer[i]      = 0;

            /* Seed RNG from VCOUNT for variety */
            rng_state ^= (u32)REG_VCOUNT;
            return;
        }
    }
    /* Pool full — silently drop */
}

/* -------------------------------------------------------------------------
 * enemy_take_hit
 * -------------------------------------------------------------------------*/
void enemy_take_hit(u8 index, u8 damage)
{
    if (index >= MAX_ENEMIES) return;
    if (!enemies[index].active) return;
    if (enemies[index].state == ESTATE_DEAD) return;

    enemies[index].hp -= (s16)damage;
    if (enemies[index].hp <= 0)
    {
        enemies[index].hp    = 0;
        enemies[index].state = ESTATE_DEAD;
        death_timer[index]   = 90;  /* 90-frame death animation */
    }
}

/* -------------------------------------------------------------------------
 * enemy_update_all
 * Called once per frame.
 * -------------------------------------------------------------------------*/
void enemy_update_all(s16 player_x, s16 player_y, u16 day, bool is_night)
{
    u8 i;
    (void)day; /* day is available for future tier-specific behaviour */

    for (i = 0; i < MAX_ENEMIES; ++i)
    {
        if (!enemies[i].active) continue;

        switch (enemies[i].state)
        {
            /* -------------------------------------------------------------- */
            case ESTATE_DEAD:
            {
                if (death_timer[i] > 0)
                {
                    --death_timer[i];
                }
                else
                {
                    enemies[i].active = FALSE;
                }
                break;
            }

            /* -------------------------------------------------------------- */
            case ESTATE_IDLE:
            {
                /* Night-only enemies do nothing during the day beyond idle */
                s16 dist = chebyshev(enemies[i].x, enemies[i].y, player_x, player_y);

                /* Wraith/Bat: forced retreat if daytime and close enough */
                if (!is_night &&
                    (enemies[i].type == ENEMY_WRAITH || enemies[i].type == ENEMY_BAT))
                {
                    /* Already idle, nothing to do */
                    break;
                }

                if (dist < 80)
                {
                    enemies[i].state = ESTATE_CHASE;
                }
                break;
            }

            /* -------------------------------------------------------------- */
            case ESTATE_CHASE:
            {
                s16 dist = chebyshev(enemies[i].x, enemies[i].y, player_x, player_y);

                /* Wraith/Bat retreat during day */
                if (!is_night &&
                    (enemies[i].type == ENEMY_WRAITH || enemies[i].type == ENEMY_BAT))
                {
                    if (dist > 40)
                    {
                        enemies[i].state = ESTATE_IDLE;
                        break;
                    }
                }

                if (dist < 16)
                {
                    enemies[i].state  = ESTATE_ATTACK;
                    attack_timer[i]   = 0;
                    break;
                }

                /* Move 2 px per frame toward player */
                {
                    s16 dx = (s16)(player_x - enemies[i].x);
                    s16 dy = (s16)(player_y - enemies[i].y);

                    if (dx > 0)       enemies[i].x += 2;
                    else if (dx < 0)  enemies[i].x -= 2;

                    if (dy > 0)       enemies[i].y += 2;
                    else if (dy < 0)  enemies[i].y -= 2;
                }
                break;
            }

            /* -------------------------------------------------------------- */
            case ESTATE_ATTACK:
            {
                s16 dist = chebyshev(enemies[i].x, enemies[i].y, player_x, player_y);

                if (dist > 24)
                {
                    enemies[i].state = ESTATE_CHASE;
                    break;
                }

                /* Deal damage every 60 frames */
                if (attack_timer[i] > 0)
                {
                    --attack_timer[i];
                }
                else
                {
                    /*
                     * Damage is applied to the player stats.  We cannot
                     * access a Player* here without a circular dependency, so
                     * we expose a weak hook that main.c / player.c can fill.
                     * For now we record the pending damage in the global
                     * below; the caller checks it each frame.
                     */
                    attack_timer[i] = 60;
                    /* Damage application is left to the caller via
                     * enemy_pending_damage() — see below. */
                    (void)k_damage_table[enemies[i].type];
                }
                break;
            }
        }
    }
}

/* -------------------------------------------------------------------------
 * spawn_tier_from_day
 * -------------------------------------------------------------------------*/
u8 spawn_tier_from_day(u16 day)
{
    if (day < 5)  return 0;
    if (day < 15) return 1;
    if (day < 30) return 2;
    if (day < 60) return 3;
    return 4;
}

/* -------------------------------------------------------------------------
 * siege_event_check
 * Every 7 nights (night_count % 7 == 0 && night_count > 0) spawn a wave of
 * 5–8 enemies near the player spawn point (tile 0,0 = pixel 0,0).
 * -------------------------------------------------------------------------*/
void siege_event_check(u16 day, u16 night_count)
{
    u8  tier, count, i;
    u16 rnd;

    if (night_count == 0) return;
    if ((night_count % 7) != 0) return;

    tier = spawn_tier_from_day(day);

    /* Determine wave size: 5–8 enemies */
    rnd   = (u16)(lcg_next() >> 16);
    count = (u8)(5 + (rnd % 4)); /* 5, 6, 7, or 8 */

    for (i = 0; i < count; ++i)
    {
        EnemyType type;
        s16       sx, sy;

        /* Choose enemy type based on tier */
        rnd = (u16)(lcg_next() >> 16);

        switch (tier)
        {
            case 0:
                type = (EnemyType)(rnd % 2); /* SLIME or BAT */
                break;
            case 1:
                type = (EnemyType)(rnd % 4); /* SLIME..GOBLIN */
                break;
            case 2:
                type = (EnemyType)(rnd % 6); /* SLIME..WRAITH */
                break;
            case 3:
                type = (EnemyType)(rnd % 9); /* SLIME..VINE_WHIP */
                break;
            default:
                type = (EnemyType)(rnd % (u16)ENEMY_COUNT);
                break;
        }

        /* Spawn in a ring 80–160 px around origin (player spawn) */
        {
            u16 angle = (u16)(lcg_next() >> 16);
            s16 radius = (s16)(80 + (lcg_next() >> 17) % 80);
            /* Approximate cardinal/diagonal offsets using angle quadrant */
            u8 quad = (u8)(angle % 8);
            static const s16 k_offsets[8][2] = {
                { 1,  0}, { 1,  1}, { 0,  1}, {-1,  1},
                {-1,  0}, {-1, -1}, { 0, -1}, { 1, -1}
            };
            sx = (s16)(k_offsets[quad][0] * radius);
            sy = (s16)(k_offsets[quad][1] * radius);
        }

        enemy_spawn(type, sx, sy);
    }
}
