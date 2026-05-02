#include "hollow_throne.h"
#include "../systems/monsters.h"

/* ---- hollow_throne_init -------------------------------------------------- */
/* Fill all 3 floors with CORRUPTION tile (value 11). Reset floor and flag.   */
void hollow_throne_init(Sanctum *s)
{
    u8  floor_idx;
    u16 tile_idx;

    for (floor_idx = 0; floor_idx < DUNGEON_FLOORS; floor_idx++) {
        for (tile_idx = 0; tile_idx < DUNGEON_FLOOR_W * DUNGEON_FLOOR_H; tile_idx++) {
            s->floors[floor_idx].tiles[tile_idx] = 11; /* CORRUPTION tile */
        }
    }

    s->current_floor = 0;
    s->boss_defeated = FALSE;
}

/* ---- entry_gate_check ---------------------------------------------------- */
/* Scan all 24 inventory slots for shard IDs 20-24 (Verdant=20, Tide=21,     */
/* Frost=22, Veil=23, Ember=24). Return TRUE only when all 5 are present      */
/* with quantity >= 1.                                                          */
bool entry_gate_check(const Inventory *inv)
{
    /* Shard ItemType values (enum positions 20-24) */
    static const u8 SHARD_IDS[5] = { 20, 21, 22, 23, 24 };

    u8   shard;
    u8   slot;
    bool found;

    for (shard = 0; shard < 5; shard++) {
        found = FALSE;
        for (slot = 0; slot < INVENTORY_SLOTS; slot++) {
            if ((u8)inv->slots[slot].type == SHARD_IDS[shard] &&
                inv->slots[slot].quantity >= 1) {
                found = TRUE;
                break;
            }
        }
        if (!found) {
            return FALSE;
        }
    }

    return TRUE;
}

/* ---- corruption_tile_drain ----------------------------------------------- */
/* Every 60 frames decrement a static HP placeholder by 1.                    */
void corruption_tile_drain(s16 player_x, s16 player_y)
{
    static u8  drain_timer = 0;
    static s16 stored_hp   = 100;

    (void)player_x;
    (void)player_y;

    drain_timer++;
    if (drain_timer >= 60) {
        stored_hp--;
        drain_timer = 0;
    }
}

/* ---- echo_enemy_update --------------------------------------------------- */
/* Circular direction cycling every 30 frames. Move 2px per direction step.   */
void echo_enemy_update(Enemy *e, s16 player_x, s16 player_y)
{
    static u8 dir_index  = 0;
    static u8 move_timer = 0;

    (void)player_x;
    (void)player_y;

    if (!e->active) {
        return;
    }

    move_timer++;
    if (move_timer >= 30) {
        dir_index  = (dir_index + 1) % 4;
        move_timer = 0;
    }

    /* 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT */
    switch (dir_index) {
        case 0: e->y -= 2; break;
        case 1: e->x += 2; break;
        case 2: e->y += 2; break;
        case 3: e->x -= 2; break;
        default: break;
    }

    /* Clamp x to [0, 240], y to [0, 160] */
    if (e->x < 0)   e->x = 0;
    if (e->x > 240) e->x = 240;
    if (e->y < 0)   e->y = 0;
    if (e->y > 160) e->y = 160;
}

/* ---- hollow_sovereign_update --------------------------------------------- */
/* Phase 0: move 2px toward player. At 50% HP transition to phase 1.          */
/* Phase 1: 240-frame cycle of 4 attack patterns.                             */
void hollow_sovereign_update(Boss *boss, s16 player_x, s16 player_y, u16 *keys)
{
    static u8 pattern_timer = 0;
    s16 dx;
    s16 dy;

    (void)keys;

    if (!boss->active) {
        return;
    }

    if (boss->phase == 0) {
        /* Move 2px toward player */
        dx = player_x - boss->x;
        dy = player_y - boss->y;

        if (dx > 0)      boss->x += 2;
        else if (dx < 0) boss->x -= 2;

        if (dy > 0)      boss->y += 2;
        else if (dy < 0) boss->y -= 2;

        /* Transition to phase 1 at 50% HP */
        if (boss->hp <= (boss->max_hp / 2)) {
            boss->phase   = 1;
            pattern_timer = 0;
        }
    } else {
        /* Phase 1: 240-frame attack pattern cycle */
        pattern_timer++;
        if (pattern_timer >= 240) {
            pattern_timer = 0;
        }

        dx = player_x - boss->x;
        dy = player_y - boss->y;

        if (pattern_timer < 60) {
            /* Pattern 0 (0-59): move toward player at 3px */
            if (dx > 0)      boss->x += 3;
            else if (dx < 0) boss->x -= 3;

            if (dy > 0)      boss->y += 3;
            else if (dy < 0) boss->y -= 3;
        } else if (pattern_timer < 120) {
            /* Pattern 1 (60-119): stay still */
            (void)0;
        } else if (pattern_timer < 180) {
            /* Pattern 2 (120-179): move away from player 2px */
            if (dx > 0)      boss->x -= 2;
            else if (dx < 0) boss->x += 2;

            if (dy > 0)      boss->y -= 2;
            else if (dy < 0) boss->y += 2;
        } else {
            /* Pattern 3 (180-239): move toward player 1px */
            if (dx > 0)      boss->x += 1;
            else if (dx < 0) boss->x -= 1;

            if (dy > 0)      boss->y += 1;
            else if (dy < 0) boss->y -= 1;
        }
    }

    /* Check defeat condition */
    if (boss_is_defeated()) {
        boss->active = FALSE;
    }
}
