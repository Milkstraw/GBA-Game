#include <tonc.h>
#include "player.h"
#include "input.h"
#include "tile_engine.h"
#include "demo_map.h"
#include "constants.h"

Player g_player;

/*
 * Sprite tile layout in OAM charblock (4bpp, 1D mapping).
 * Each animation frame of the 16×24 player uses 6 hardware 8×8 tiles:
 *   - 4 tiles for the upper 16×16 OAM entry
 *   - 2 tiles for the lower 16×8  OAM entry
 * 4 directions × 3 frames × 6 tiles = 72 tiles total.
 */
#define PLAYER_SPR_BASE        0u  /* first OBJ tile index */
#define PLAYER_TILES_PER_FRAME 6u  /* hw tiles consumed per anim frame */
#define PLAYER_ANIM_PERIOD     8u  /* advance anim_frame every N game frames */

void player_init(void)
{
    g_player.x          = 6;
    g_player.y          = 6;
    g_player.px         = 6 * TILE_W;
    g_player.py         = 6 * TILE_H;
    g_player.dir        = DIR_DOWN;
    g_player.anim_frame = 0;
    g_player.anim_timer = 0;
    g_player.is_moving  = 0;

    /* Enable OBJ display and 1D tile mapping */
    REG_DISPCNT |= DCNT_OBJ | DCNT_OBJ_1D;

    /* Hide OAM entries 0 and 1 until first draw */
    obj_mem[0].attr0 = ATTR0_HIDE;
    obj_mem[1].attr0 = ATTR0_HIDE;
}

void player_update(void)
{
    s8 dx = 0, dy = 0;
    u8 dir_held = 0;

    /* Determine intended direction from held keys */
    if (KEY_HELD(KEY_UP))    { dy = -1; g_player.dir = DIR_UP;    dir_held = 1; }
    if (KEY_HELD(KEY_DOWN))  { dy =  1; g_player.dir = DIR_DOWN;  dir_held = 1; }
    if (KEY_HELD(KEY_LEFT))  { dx = -1; g_player.dir = DIR_LEFT;  dir_held = 1; }
    if (KEY_HELD(KEY_RIGHT)) { dx =  1; g_player.dir = DIR_RIGHT; dir_held = 1; }

    g_player.is_moving = dir_held;

    if (dir_held) {
        /* Advance animation timer; move one tile when timer fires */
        g_player.anim_timer++;
        if (g_player.anim_timer >= PLAYER_ANIM_PERIOD) {
            g_player.anim_timer = 0;
            g_player.anim_frame = (u8)((g_player.anim_frame + 1u) % 3u);

            /* Collision check before committing move */
            u8 nx = (u8)((s8)g_player.x + dx);
            u8 ny = (u8)((s8)g_player.y + dy);
            if (nx < MAP_W && ny < MAP_H) {
                u8 tile = map_tile_at(nx, ny);
                if (!(g_collision_flags[tile] & COL_SOLID)) {
                    g_player.x  = nx;
                    g_player.y  = ny;
                    g_player.px = (u8)(nx * TILE_W);
                    g_player.py = (u8)(ny * TILE_H);
                }
            }
        }
    } else {
        /* Standing still: reset animation */
        g_player.anim_frame = 0;
        g_player.anim_timer = 0;
    }

    /* Interact: check tile in front of player on A press */
    if (KEY_PRESSED(KEY_A)) {
        s8 ix = (s8)g_player.x;
        s8 iy = (s8)g_player.y;
        switch (g_player.dir) {
        case DIR_DOWN:  iy++; break;
        case DIR_UP:    iy--; break;
        case DIR_LEFT:  ix--; break;
        case DIR_RIGHT: ix++; break;
        default: break;
        }
        if (ix >= 0 && ix < (s8)MAP_W && iy >= 0 && iy < (s8)MAP_H) {
            u8 tile = map_tile_at((u8)ix, (u8)iy);
            if (g_collision_flags[tile] & COL_INTERACT) {
                player_on_interact((u8)ix, (u8)iy);
            }
        }
    }
}

void player_draw(void)
{
    u16 tile = (u16)(PLAYER_SPR_BASE
                     + ((u16)g_player.dir * 3u + g_player.anim_frame)
                       * PLAYER_TILES_PER_FRAME);

    /* OAM 0 — upper 16×16 sprite (shape=square, size=1) */
    obj_mem[0].attr0 = (u16)(g_player.py & 0xFFu) | ATTR0_SQUARE;
    obj_mem[0].attr1 = (u16)(g_player.px & 0x1FFu) | ATTR1_SIZE_16;
    obj_mem[0].attr2 = tile;  /* SPR PAL 0: ATTR2_PALBANK(0) = 0 */

    /* OAM 1 — lower 16×8 sprite (shape=wide, size=0) */
    obj_mem[1].attr0 = (u16)((g_player.py + 16u) & 0xFFu) | ATTR0_WIDE;
    obj_mem[1].attr1 = (u16)(g_player.px & 0x1FFu);  /* size=0: ATTR1_SIZE_8 = 0 */
    obj_mem[1].attr2 = (u16)(tile + 4u);
}

__attribute__((weak)) void player_on_interact(u8 tile_x, u8 tile_y)
{
    (void)tile_x;
    (void)tile_y;
}
