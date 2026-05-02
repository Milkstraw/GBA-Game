/*
 * combat.c — Melee, ranged, and projectile combat for HollowShore
 * Wave 5b implementation
 */

#include "combat.h"
#include "monsters.h"
#include "../core/types.h"
#include "../player/player.h"

/* Section bounds (pixels) — SECTION_TILES_W/H * TILE_SIZE */
#define SECTION_W_PX  2560   /* 160 tiles * 16 px */
#define SECTION_H_PX  2560

/* Projectile pool — extern declared in combat.h */
Projectile projectiles[MAX_PROJECTILES];

/* -------------------------------------------------------------------------
 * attack_melee
 * Compute a 32x32 hitbox 16px in front of the player, then damage any active
 * enemy whose position falls inside it.
 * -------------------------------------------------------------------------*/
void attack_melee(s16 player_x, s16 player_y, u8 facing)
{
    s16 hx, hy;

    /* Place hitbox 16 px ahead of player centre */
    switch (facing)
    {
        case FACE_UP:
            hx = player_x;
            hy = player_y - 16;
            break;
        case FACE_DOWN:
            hx = player_x;
            hy = player_y + 16;
            break;
        case FACE_LEFT:
            hx = player_x - 16;
            hy = player_y;
            break;
        case FACE_RIGHT:
        default:
            hx = player_x + 16;
            hy = player_y;
            break;
    }

    /* Half-extents of the 32x32 hitbox */
    s16 hx_min = hx - 16;
    s16 hx_max = hx + 16;
    s16 hy_min = hy - 16;
    s16 hy_max = hy + 16;

    u8 i;
    for (i = 0; i < MAX_ENEMIES; ++i)
    {
        if (!enemies[i].active) continue;
        if (enemies[i].state == ESTATE_DEAD) continue;

        if (enemies[i].x >= hx_min && enemies[i].x <= hx_max &&
            enemies[i].y >= hy_min && enemies[i].y <= hy_max)
        {
            enemy_take_hit(i, 8);
        }
    }
}

/* -------------------------------------------------------------------------
 * attack_ranged
 * Spawn a projectile from the first free slot, aimed in the player's facing
 * direction.  Damage = 5.
 * -------------------------------------------------------------------------*/
void attack_ranged(s16 player_x, s16 player_y, u8 facing)
{
    u8 i;
    for (i = 0; i < MAX_PROJECTILES; ++i)
    {
        if (!projectiles[i].active)
        {
            projectiles[i].x      = player_x;
            projectiles[i].y      = player_y;
            projectiles[i].damage = 5;
            projectiles[i].active = TRUE;

            switch (facing)
            {
                case FACE_UP:
                    projectiles[i].dx = 0;
                    projectiles[i].dy = -3;
                    break;
                case FACE_DOWN:
                    projectiles[i].dx = 0;
                    projectiles[i].dy = 3;
                    break;
                case FACE_LEFT:
                    projectiles[i].dx = -3;
                    projectiles[i].dy = 0;
                    break;
                case FACE_RIGHT:
                default:
                    projectiles[i].dx = 3;
                    projectiles[i].dy = 0;
                    break;
            }
            return; /* only one projectile per call */
        }
    }
    /* Pool full — silently drop */
}

/* -------------------------------------------------------------------------
 * projectile_update
 * Move each active projectile, deactivate on out-of-bounds, and check enemy
 * collisions (16x16 enemy hit radius).
 * -------------------------------------------------------------------------*/
void projectile_update(void)
{
    u8 i, j;
    for (i = 0; i < MAX_PROJECTILES; ++i)
    {
        if (!projectiles[i].active) continue;

        projectiles[i].x += projectiles[i].dx;
        projectiles[i].y += projectiles[i].dy;

        /* Bounds check against the section pixel dimensions */
        if (projectiles[i].x < 0 || projectiles[i].x >= SECTION_W_PX ||
            projectiles[i].y < 0 || projectiles[i].y >= SECTION_H_PX)
        {
            projectiles[i].active = FALSE;
            continue;
        }

        /* Enemy collision — treat each enemy as a 16x16 box */
        for (j = 0; j < MAX_ENEMIES; ++j)
        {
            if (!enemies[j].active) continue;
            if (enemies[j].state == ESTATE_DEAD) continue;

            s16 ex_min = enemies[j].x - 8;
            s16 ex_max = enemies[j].x + 8;
            s16 ey_min = enemies[j].y - 8;
            s16 ey_max = enemies[j].y + 8;

            if (projectiles[i].x >= ex_min && projectiles[i].x <= ex_max &&
                projectiles[i].y >= ey_min && projectiles[i].y <= ey_max)
            {
                enemy_take_hit(j, projectiles[i].damage);
                projectiles[i].active = FALSE;
                break;
            }
        }
    }
}
