#include "frostspire.h"
#include "bosses.h"
#include "../systems/monsters.h"

#define TILE_ICE  10
#define TILE_ROCK  4

/* ---- frostspire_init -----------------------------------------------------
 * Fill all 3 floors with ICE tiles (value 10).
 * Every 7th tile position gets a ROCK patch (value 4).
 * Reset floor index and boss state.
 * -------------------------------------------------------------------------- */
void frostspire_init(Sanctum *s)
{
    u8 f;
    u16 i;

    for (f = 0; f < DUNGEON_FLOORS; f++) {
        for (i = 0; i < DUNGEON_FLOOR_W * DUNGEON_FLOOR_H; i++) {
            if (i % 7 == 0) {
                s->floors[f].tiles[i] = TILE_ROCK;
            } else {
                s->floors[f].tiles[i] = TILE_ICE;
            }
        }
    }

    s->current_floor = 0;
    s->boss_defeated  = FALSE;
}

/* ---- ice_momentum --------------------------------------------------------
 * Add 2 to velocity in the facing direction, then clamp each component
 * to the range [-6, 6].
 *   0 = UP    -> vy -= 2
 *   1 = DOWN  -> vy += 2
 *   2 = LEFT  -> vx -= 2
 *   3 = RIGHT -> vx += 2
 * -------------------------------------------------------------------------- */
void ice_momentum(s16 *vx, s16 *vy, u8 facing)
{
    switch (facing) {
        case 0: *vy -= 2; break;   /* UP    */
        case 1: *vy += 2; break;   /* DOWN  */
        case 2: *vx -= 2; break;   /* LEFT  */
        case 3: *vx += 2; break;   /* RIGHT */
        default: break;
    }

    /* Clamp vx to [-6, 6] */
    if (*vx < -6) *vx = -6;
    if (*vx >  6) *vx =  6;

    /* Clamp vy to [-6, 6] */
    if (*vy < -6) *vy = -6;
    if (*vy >  6) *vy =  6;
}

/* ---- glacial_warden_update -----------------------------------------------
 * Move 2 px/frame toward the player.
 * At 50 % HP (phase 0 only): enter phase 1 and spawn two SLIME enemies.
 * Check defeat condition and deactivate if dead.
 * -------------------------------------------------------------------------- */
void glacial_warden_update(Boss *boss, s16 player_x, s16 player_y)
{
    if (!boss->active) {
        return;
    }

    /* Move 2 px per frame toward the player on each axis independently */
    if (boss->x < player_x) {
        boss->x += 2;
    } else if (boss->x > player_x) {
        boss->x -= 2;
    }

    if (boss->y < player_y) {
        boss->y += 2;
    } else if (boss->y > player_y) {
        boss->y -= 2;
    }

    /* Phase transition at 50 % HP */
    if (boss->phase == 0 && boss->hp <= boss->max_hp / 2) {
        boss->phase = 1;
        enemy_spawn(ENEMY_SLIME, boss->x - 20, boss->y);
        enemy_spawn(ENEMY_SLIME, boss->x + 20, boss->y);
    }

    /* Defeat check */
    if (boss_is_defeated()) {
        boss->active = FALSE;
    }
}
