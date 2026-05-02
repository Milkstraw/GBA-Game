/* emberdepth.c — Sanctum 5: The Emberdepth (Volcanic, NE)
 * Drops: Ember Shard + Magma Core
 */

#include "emberdepth.h"

/* ---- Tile IDs -------------------------------------------------------------- */
#define TILE_ROCK  4
#define TILE_LAVA  9

/* ---- Module-local state ---------------------------------------------------- */

/* Runtime flat tile map for lava tracking (mirrors current floor, mutable). */
static u8  floor_tiles[DUNGEON_FLOOR_W * DUNGEON_FLOOR_H];

/* Shadow HP used by lava_tile_damage (HP integration done externally). */
static s16 stored_hp    = 100;

/* Frame counter for lava damage pacing. */
static u8  damage_timer = 0;

/* ============================================================================
 * emberdepth_init
 * ============================================================================
 * Floor 0 : even tile indices  -> ROCK(4), odd indices -> LAVA(9).
 * Floors 1–2 : every tile where (index % 3 == 2) -> LAVA(9), else ROCK(4).
 *              (Denser lava than floor 0.)
 * Sets s->current_floor = 0 and s->boss_defeated = FALSE.
 * Seeds the module-local floor_tiles[] from floor 0.
 * =========================================================================== */
void emberdepth_init(Sanctum *s)
{
    u16 i;
    u8  f;

    for (f = 0; f < DUNGEON_FLOORS; f++) {
        for (i = 0; i < (u16)(DUNGEON_FLOOR_W * DUNGEON_FLOOR_H); i++) {
            if (f == 0) {
                s->floors[f].tiles[i] = (i % 2 == 0) ? TILE_ROCK : TILE_LAVA;
            } else {
                /* Floors 1 and 2: denser lava — every 3rd tile is LAVA */
                s->floors[f].tiles[i] = (i % 3 == 2) ? TILE_LAVA : TILE_ROCK;
            }
        }
    }

    /* Seed the runtime lava map from floor 0 */
    for (i = 0; i < (u16)(DUNGEON_FLOOR_W * DUNGEON_FLOOR_H); i++) {
        floor_tiles[i] = s->floors[0].tiles[i];
    }

    s->current_floor = 0;
    s->boss_defeated = FALSE;

    /* Reset module timers */
    damage_timer = 0;
    stored_hp    = 100;
}

/* ============================================================================
 * lava_tile_damage
 * ============================================================================
 * Called once per frame.  Increments damage_timer every call.
 * Every 30 frames: compute tile_x = player_x / 8, tile_y = player_y / 8,
 * tile_index = tile_y * 30 + tile_x.  If floor_tiles[tile_index] == LAVA(9)
 * decrement stored_hp by 1 and reset the timer.
 * =========================================================================== */
void lava_tile_damage(s16 player_x, s16 player_y)
{
    damage_timer++;

    if (damage_timer >= 30) {
        u8  tile_x;
        u8  tile_y;
        u16 tile_index;

        damage_timer = 0;

        tile_x = (u8)(player_x / 8);
        tile_y = (u8)(player_y / 8);

        /* Clamp to floor bounds */
        if (tile_x >= DUNGEON_FLOOR_W) tile_x = DUNGEON_FLOOR_W - 1;
        if (tile_y >= DUNGEON_FLOOR_H) tile_y = DUNGEON_FLOOR_H - 1;

        tile_index = (u16)((u16)tile_y * DUNGEON_FLOOR_W + (u16)tile_x);

        if (floor_tiles[tile_index] == TILE_LAVA) {
            stored_hp--;
        }
    }
}

/* ============================================================================
 * lava_cool
 * ============================================================================
 * Set the floor_tiles entry at y*30+x to ROCK(4).
 * =========================================================================== */
void lava_cool(u8 x, u8 y)
{
    u16 index;

    if (x >= DUNGEON_FLOOR_W) return;
    if (y >= DUNGEON_FLOOR_H) return;

    index = (u16)((u16)y * DUNGEON_FLOOR_W + (u16)x);
    floor_tiles[index] = TILE_ROCK;
}

/* ============================================================================
 * golem_enemy_update
 * ============================================================================
 * Slow golem — moves 1 px/frame toward the player.
 * If the golem is within 16 px on BOTH axes, enter ESTATE_ATTACK.
 * =========================================================================== */
void golem_enemy_update(Enemy *e, s16 player_x, s16 player_y)
{
    s16 dx;
    s16 dy;

    if (!e->active) return;

    dx = player_x - e->x;
    dy = player_y - e->y;

    /* Move 1 px/frame toward the player on each axis */
    if      (dx > 0) e->x++;
    else if (dx < 0) e->x--;

    if      (dy > 0) e->y++;
    else if (dy < 0) e->y--;

    /* Absolute value for distance check */
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    if (dx < 16 && dy < 16) {
        e->state = ESTATE_ATTACK;
    }
}

/* ============================================================================
 * ember_titan_update
 * ============================================================================
 * Boss update for the Ember Titan:
 *
 *  - Moves 2 px/frame toward the player (both axes).
 *  - Phase transition: when boss->hp <= boss->max_hp/2 and boss->phase==0,
 *    set boss->phase = 1.
 *  - Phase 1 lava eruption: keep a static lava_timer; increment each call.
 *    Every 120 frames mark the tile under the boss as LAVA(9) in floor_tiles.
 *  - If boss_is_defeated() returns TRUE, set boss->active = FALSE.
 * =========================================================================== */
void ember_titan_update(Boss *boss, s16 player_x, s16 player_y)
{
    static u8 lava_timer = 0;
    s16 dx;
    s16 dy;

    if (!boss->active) return;

    /* Move 2 px/frame toward the player */
    dx = player_x - boss->x;
    dy = player_y - boss->y;

    if      (dx > 0) boss->x += 2;
    else if (dx < 0) boss->x -= 2;

    if      (dy > 0) boss->y += 2;
    else if (dy < 0) boss->y -= 2;

    /* Phase transition at 50% HP */
    if (boss->phase == 0 && boss->hp <= boss->max_hp / 2) {
        boss->phase = 1;
    }

    /* Phase 1: every 120 frames mark tile under boss as LAVA */
    if (boss->phase == 1) {
        u8  tile_x;
        u8  tile_y;
        u16 tile_index;

        lava_timer++;
        if (lava_timer >= 120) {
            lava_timer = 0;

            tile_x = (u8)(boss->x / 8);
            tile_y = (u8)(boss->y / 8);

            if (tile_x >= DUNGEON_FLOOR_W) tile_x = DUNGEON_FLOOR_W - 1;
            if (tile_y >= DUNGEON_FLOOR_H) tile_y = DUNGEON_FLOOR_H - 1;

            tile_index = (u16)((u16)tile_y * DUNGEON_FLOOR_W + (u16)tile_x);
            floor_tiles[tile_index] = TILE_LAVA;
        }
    }

    /* Defeat check */
    if (boss_is_defeated()) {
        boss->active = FALSE;
    }
}
