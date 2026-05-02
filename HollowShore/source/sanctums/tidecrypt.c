#include "tidecrypt.h"
#include "bosses.h"

#define TILE_DIRT  1
#define TILE_WATER 7

/* ---- tidecrypt_init --------------------------------------------------------
   floors[0] and floors[1]: all TILE_DIRT
   floors[2]:               alternating rows of TILE_DIRT / TILE_WATER
   --------------------------------------------------------------------------- */
void tidecrypt_init(Sanctum *s) {
    u16 col, row, idx;

    /* Floor 0 — all dirt */
    for (idx = 0; idx < DUNGEON_FLOOR_W * DUNGEON_FLOOR_H; idx++) {
        s->floors[0].tiles[idx] = TILE_DIRT;
    }

    /* Floor 1 — all dirt */
    for (idx = 0; idx < DUNGEON_FLOOR_W * DUNGEON_FLOOR_H; idx++) {
        s->floors[1].tiles[idx] = TILE_DIRT;
    }

    /* Floor 2 — alternating rows: even rows = TILE_DIRT, odd rows = TILE_WATER */
    for (row = 0; row < DUNGEON_FLOOR_H; row++) {
        u8 tile = (row % 2 == 0) ? TILE_DIRT : TILE_WATER;
        for (col = 0; col < DUNGEON_FLOOR_W; col++) {
            s->floors[2].tiles[row * DUNGEON_FLOOR_W + col] = tile;
        }
    }

    s->current_floor = 0;
    s->boss_defeated  = FALSE;
}

/* ---- tide_update -----------------------------------------------------------
   Toggle tide state every 300 frames.
   --------------------------------------------------------------------------- */
void tide_update(void) {
    static u16  s_frame_counter = 0;
    static bool s_tide_high     = FALSE;

    s_frame_counter++;
    if (s_frame_counter >= 300) {
        s_frame_counter = 0;
        s_tide_high     = s_tide_high ? FALSE : TRUE;
        /* Tile-swap handled by caller; nothing further to do here. */
    }
}

/* ---- drowned_warden_update -------------------------------------------------
   Phase 0: moves 2 px/frame toward player on each axis.
   Phase 1 (HP <= max_hp/2): speed rises to 3 px/frame.
   Deactivates when boss_is_defeated() returns TRUE.
   --------------------------------------------------------------------------- */
void drowned_warden_update(Boss *boss, s16 player_x, s16 player_y) {
    static s16 s_speed = 2;

    if (!boss->active) {
        return;
    }

    /* Phase transition: enter phase 1 at or below half HP */
    if (boss->phase == 0 && boss->hp <= boss->max_hp / 2) {
        boss->phase = 1;
        s_speed     = 3;
    }

    /* Move toward player on X axis */
    if (boss->x < player_x) {
        boss->x += s_speed;
        if (boss->x > player_x) {
            boss->x = player_x;
        }
    } else if (boss->x > player_x) {
        boss->x -= s_speed;
        if (boss->x < player_x) {
            boss->x = player_x;
        }
    }

    /* Move toward player on Y axis */
    if (boss->y < player_y) {
        boss->y += s_speed;
        if (boss->y > player_y) {
            boss->y = player_y;
        }
    } else if (boss->y > player_y) {
        boss->y -= s_speed;
        if (boss->y < player_y) {
            boss->y = player_y;
        }
    }

    /* Deactivate when defeated */
    if (boss_is_defeated()) {
        boss->active = FALSE;
    }
}
