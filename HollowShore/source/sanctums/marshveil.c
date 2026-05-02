/*
 * marshveil.c — Sanctum 4: The Marshveil (Marshland, E)
 * Drops: Veil Shard + Spectral Dust
 */

#include "marshveil.h"
#include "../gba.h"

/* =========================================================================
 * marshveil_init
 * =========================================================================
 * Fill all three DungeonFloor tile arrays with DIRT (tile id 1).
 * Scatter WATER (tile id 7) at every 11th index.
 * Reset current_floor and boss_defeated.
 * ========================================================================= */
void marshveil_init(Sanctum *s)
{
    u16 f, i;
    u16 total = (u16)(DUNGEON_FLOOR_W * DUNGEON_FLOOR_H);

    for (f = 0; f < DUNGEON_FLOORS; f++)
    {
        for (i = 0; i < total; i++)
        {
            if ((i % 11) == 0)
                s->floors[f].tiles[i] = 7; /* WATER */
            else
                s->floors[f].tiles[i] = 1; /* DIRT  */
        }
    }

    s->current_floor  = 0;
    s->boss_defeated  = FALSE;
}

/* =========================================================================
 * fog_window_update
 * =========================================================================
 * Configure GBA window 0 around the torch tile position.
 * Pixel origin: px = torch_x * 8, py = torch_y * 8.
 * Window extents: [px-40 .. px+40] x [py-40 .. py+40], clamped to screen.
 *
 * REG_WIN0H = (left << 8) | right
 * REG_WIN0V = (top  << 8) | bottom
 * REG_WININ = 0x3F  — show all layers inside window
 * REG_WINOUT = 0x00 — hide all outside window
 * REG_DISPCNT |= WIN0_ENABLE
 * ========================================================================= */
void fog_window_update(u8 torch_x, u8 torch_y)
{
    s16 px, py;
    s16 left, right, top, bottom;

    px = (s16)((s16)torch_x * 8);
    py = (s16)((s16)torch_y * 8);

    left   = (s16)(px - 40);
    right  = (s16)(px + 40);
    top    = (s16)(py - 40);
    bottom = (s16)(py + 40);

    /* Clamp horizontal */
    if (left  < 0)   left  = 0;
    if (left  > 240) left  = 240;
    if (right < 0)   right = 0;
    if (right > 240) right = 240;

    /* Clamp vertical */
    if (top    < 0)   top    = 0;
    if (top    > 160) top    = 160;
    if (bottom < 0)   bottom = 0;
    if (bottom > 160) bottom = 160;

    REG_WIN0H  = (u16)(((u16)left << 8) | (u16)right);
    REG_WIN0V  = (u16)(((u16)top  << 8) | (u16)bottom);
    REG_WININ  = 0x3F;
    REG_WINOUT = 0x00;
    REG_DISPCNT |= WIN0_ENABLE;
}

/* =========================================================================
 * mimic_enemy_update
 * =========================================================================
 * Move 2 px/frame toward the player on each axis independently.
 * Transition to ESTATE_ATTACK when within 16 px on both axes.
 * ========================================================================= */
void mimic_enemy_update(Enemy *e, s16 player_x, s16 player_y)
{
    s16 dx, dy;
    s16 adx, ady;

    if (!e->active)
        return;

    dx = (s16)(player_x - e->x);
    dy = (s16)(player_y - e->y);

    adx = (dx < 0) ? (s16)(-dx) : dx;
    ady = (dy < 0) ? (s16)(-dy) : dy;

    /* Move 2 px/frame toward player on each axis */
    if (dx > 0)
        e->x = (s16)(e->x + 2);
    else if (dx < 0)
        e->x = (s16)(e->x - 2);

    if (dy > 0)
        e->y = (s16)(e->y + 2);
    else if (dy < 0)
        e->y = (s16)(e->y - 2);

    /* Attack state when close enough on both axes */
    if (adx < 16 && ady < 16)
        e->state = ESTATE_ATTACK;
}

/* =========================================================================
 * bog_specter_update
 * =========================================================================
 * Move 2 px/frame toward the player.
 * At 50% HP (first time): set phase=1, start a 60-frame invisible timer.
 * While the timer is counting down, skip movement.
 * Call boss_is_defeated() each frame; if true, set boss->active = FALSE.
 * ========================================================================= */
void bog_specter_update(Boss *boss, s16 player_x, s16 player_y)
{
    static u8 invisible_timer = 0;
    s16 dx, dy;

    if (!boss->active)
        return;

    /* Phase transition: enter phase 1 at 50% HP */
    if (boss->phase == 0 && boss->hp <= boss->max_hp / 2)
    {
        boss->phase     = 1;
        invisible_timer = 60;
    }

    /* While invisible (phase transition stun): count down and skip movement */
    if (invisible_timer > 0)
    {
        invisible_timer--;
        return;
    }

    /* Move 2 px/frame toward player on each axis */
    dx = (s16)(player_x - boss->x);
    dy = (s16)(player_y - boss->y);

    if (dx > 0)
        boss->x = (s16)(boss->x + 2);
    else if (dx < 0)
        boss->x = (s16)(boss->x - 2);

    if (dy > 0)
        boss->y = (s16)(boss->y + 2);
    else if (dy < 0)
        boss->y = (s16)(boss->y - 2);

    /* Defeat check */
    if (boss_is_defeated())
        boss->active = FALSE;
}
