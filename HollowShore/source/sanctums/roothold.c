#include "roothold.h"
#include "bosses.h"
#include "../systems/monsters.h"

/* Tile type constants */
#define TILE_GRASS  0
#define TILE_DIRT   1
#define TILE_ROCK   4

/* Dungeon movement bounds (pixel space) */
#define BOUND_X_MIN  0
#define BOUND_X_MAX  240
#define BOUND_Y_MIN  0
#define BOUND_Y_MAX  160

/* Boss phase-transition HP threshold (50%) */
#define PHASE_HP_HALF(boss)  ((boss)->max_hp / 2)

/* ---- Helpers ------------------------------------------------------------ */

static s16 abs_s16(s16 v) {
    return v < 0 ? -v : v;
}

static s16 clamp_s16(s16 v, s16 lo, s16 hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* ---- roothold_init ------------------------------------------------------ */

void roothold_init(Sanctum *s) {
    u32 i;

    /* -- Floor 0: alternating TILE_GRASS / TILE_DIRT rows ----------------- */
    for (i = 0; i < DUNGEON_FLOOR_W * DUNGEON_FLOOR_H; i++) {
        u8 row = (u8)(i / DUNGEON_FLOOR_W);
        s->floors[0].tiles[i] = (row % 2 == 0) ? TILE_GRASS : TILE_DIRT;
    }

    /* -- Floor 1: same alternating pattern with TILE_ROCK every 5 tiles --- */
    for (i = 0; i < DUNGEON_FLOOR_W * DUNGEON_FLOOR_H; i++) {
        u8 row = (u8)(i / DUNGEON_FLOOR_W);
        if (i % 5 == 0) {
            s->floors[1].tiles[i] = TILE_ROCK;
        } else {
            s->floors[1].tiles[i] = (row % 2 == 0) ? TILE_GRASS : TILE_DIRT;
        }
    }

    /* -- Floor 2: mixed forest pattern ------------------------------------ */
    /*    Tiles cycle: GRASS, DIRT, GRASS, ROCK, DIRT                       */
    {
        const u8 forest_cycle[5] = { TILE_GRASS, TILE_DIRT, TILE_GRASS, TILE_ROCK, TILE_DIRT };
        for (i = 0; i < DUNGEON_FLOOR_W * DUNGEON_FLOOR_H; i++) {
            s->floors[2].tiles[i] = forest_cycle[i % 5];
        }
    }

    s->current_floor = 0;
    s->boss_defeated = FALSE;
}

/* ---- root_warden_update ------------------------------------------------- */

void root_warden_update(Boss *boss, s16 player_x, s16 player_y) {
    s16 dx, dy, dist;

    if (!boss->active) return;
    if (boss_is_defeated()) {
        boss->active = FALSE;
        return;
    }

    /* Move 2px per frame toward player on each axis independently */
    dx = player_x - boss->x;
    dy = player_y - boss->y;

    if (dx > 0)       boss->x += 2;
    else if (dx < 0)  boss->x -= 2;

    if (dy > 0)       boss->y += 2;
    else if (dy < 0)  boss->y -= 2;

    /* Clamp to dungeon pixel bounds */
    boss->x = clamp_s16(boss->x, BOUND_X_MIN, BOUND_X_MAX);
    boss->y = clamp_s16(boss->y, BOUND_Y_MIN, BOUND_Y_MAX);

    /* Proximity check: Manhattan distance < 20px */
    dist = abs_s16(player_x - boss->x) + abs_s16(player_y - boss->y);
    if (dist < 20) {
        /* Damage handled externally; probe phase state with zero-damage call */
        boss_take_hit(0);
    }

    /* Phase 0 -> 1 transition at 50% HP: spawn two Vine Whip minions */
    if (boss->phase == 0 && boss->hp <= PHASE_HP_HALF(boss)) {
        boss->phase = 1;
        enemy_spawn(ENEMY_VINE_WHIP, (s16)(boss->x - 16), boss->y);
        enemy_spawn(ENEMY_VINE_WHIP, (s16)(boss->x + 16), boss->y);
    }

    /* Final defeat check */
    if (boss_is_defeated()) {
        boss->active = FALSE;
    }
}
