#include "player.h"
#include "../gba.h"
#include "../core/input.h"
#include "../graphics/sprites.h"
#include "../world/section.h"
#include "../world/tile.h"

/* ---- Constants ----------------------------------------------------------- */
#define WALK_SPEED    4                                    /* px/frame */
#define SPRINT_SPEED  6                                    /* px/frame */
#define SPRITE_SIZE   16                                   /* player sprite width/height in px */
#define ATTACK_FRAMES 16                                   /* attack animation length */

#define WORLD_PX_W  ((s16)(SECTION_TILES_W * TILE_SIZE))  /* total world width in px  */
#define WORLD_PX_H  ((s16)(SECTION_TILES_H * TILE_SIZE))  /* total world height in px */

/* ---- Module-level state -------------------------------------------------- */

/*
 * The active section is set by the game loop (player_set_section).
 * When NULL collision is skipped — movement is always allowed.
 * This avoids altering the fixed player_update(Player*, u16) signature.
 */
static const Section *s_section      = (const Section *)0;
static u16            s_anim_frame   = 0;
static u16            s_attack_timer = 0;

/* ---- player_set_section -------------------------------------------------- */
/*
 * Called by the game loop before entering the per-frame update.
 * Not declared in player.h (header is fixed), but defined here for
 * main.c / world layer to call via forward declaration.
 */
void player_set_section(const Section *sec)
{
    s_section = sec;
}

/* ---- Internal: tile passable at pixel coordinate ----------------------- */
static bool tile_passable_at(s16 px, s16 py)
{
    u8       tx, ty;
    TileType tt;

    if (!s_section)
        return TRUE;  /* no section loaded — allow movement */

    /* Pixel to tile coordinate */
    tx = (u8)((u16)px / (u16)TILE_SIZE);
    ty = (u8)((u16)py / (u16)TILE_SIZE);

    if (tx >= (u8)SECTION_TILES_W) tx = (u8)(SECTION_TILES_W - 1u);
    if (ty >= (u8)SECTION_TILES_H) ty = (u8)(SECTION_TILES_H - 1u);

    tt = section_get_tile(s_section, tx, ty);

    if ((u8)tt >= (u8)TILE_COUNT)
        return TRUE;   /* unknown tile type — allow */

    return tile_defs[(u8)tt].passable;
}

/* ---- player_init --------------------------------------------------------- */
void player_init(Player *p)
{
    p->x         = (s16)((SECTION_TILES_W / 2) * TILE_SIZE);
    p->y         = (s16)((SECTION_TILES_H / 2) * TILE_SIZE);
    p->facing    = FACE_DOWN;
    p->state     = PSTATE_IDLE;
    p->sprite_id = 0;

    stats_init(&p->stats);

    s_anim_frame   = 0;
    s_attack_timer = 0;
    s_section      = (const Section *)0;
}

/* ---- player_update ------------------------------------------------------- */
void player_update(Player *p, u16 keys)
{
    bool sprint;
    s16  speed;
    s16  new_x, new_y;
    bool moved;
    s16  max_x, max_y;

    /* Advance global animation counter */
    s_anim_frame++;

    /* Tick attack animation */
    if (s_attack_timer > 0)
    {
        s_attack_timer--;
        if (s_attack_timer == 0)
            p->state = PSTATE_IDLE;
    }

    /* Sprint: KEY_B held and stamina remaining */
    sprint = (bool)((keys & KEY_B) && (p->stats.stamina > 0));
    speed  = sprint ? (s16)SPRINT_SPEED : (s16)WALK_SPEED;

    /* World pixel bounds (player sprite occupies SPRITE_SIZE × SPRITE_SIZE) */
    max_x = (s16)(WORLD_PX_W - SPRITE_SIZE);
    max_y = (s16)(WORLD_PX_H - SPRITE_SIZE);

    /* Movement */
    new_x = p->x;
    new_y = p->y;
    moved = FALSE;

    if (keys & KEY_UP)
    {
        p->facing = FACE_UP;
        new_y     = (s16)(p->y - speed);
        moved     = TRUE;
    }
    else if (keys & KEY_DOWN)
    {
        p->facing = FACE_DOWN;
        new_y     = (s16)(p->y + speed);
        moved     = TRUE;
    }

    if (keys & KEY_LEFT)
    {
        p->facing = FACE_LEFT;
        new_x     = (s16)(p->x - speed);
        moved     = TRUE;
    }
    else if (keys & KEY_RIGHT)
    {
        p->facing = FACE_RIGHT;
        new_x     = (s16)(p->x + speed);
        moved     = TRUE;
    }

    if (moved)
    {
        /* Clamp to section bounds */
        if (new_x < 0)      new_x = 0;
        if (new_x > max_x)  new_x = max_x;
        if (new_y < 0)      new_y = 0;
        if (new_y > max_y)  new_y = max_y;

        /* Tile passability check at destination top-left corner */
        if (tile_passable_at(new_x, new_y))
        {
            p->x = new_x;
            p->y = new_y;

            /* Drain stamina when sprinting */
            if (sprint && p->stats.stamina > 0)
                p->stats.stamina--;
        }
    }

    /* Attack trigger: KEY_A when idle starts attack animation */
    if ((keys & KEY_A) && p->state == PSTATE_IDLE)
    {
        p->state       = PSTATE_ATTACKING;
        s_attack_timer = (u16)ATTACK_FRAMES;
    }

    /* State machine: resolve walk/idle when not in attack animation */
    if (s_attack_timer == 0)
    {
        if (moved)
            p->state = PSTATE_MOVING;
        else
            p->state = PSTATE_IDLE;
    }
}

/* ---- player_draw --------------------------------------------------------- */
/*
 * Sprite tile layout (2 walk-frames per facing direction):
 *   FACE_DOWN  → base tile  0  (frames 0, 1)
 *   FACE_UP    → base tile  4  (frames 4, 5)
 *   FACE_LEFT  → base tile  8  (frames 8, 9)
 *   FACE_RIGHT → base tile 12  (frames 12, 13)
 *
 * Walk animation toggles every 8 ticks via bit 3 of s_anim_frame.
 */
void player_draw(const Player *p, s16 cam_x, s16 cam_y)
{
    s16 sx, sy;
    u8  base_tile;
    u8  anim_offset;
    u8  tile_id;

    sx = (s16)(p->x - cam_x);
    sy = (s16)(p->y - cam_y);

    switch (p->facing)
    {
        case FACE_DOWN:  base_tile =  0; break;
        case FACE_UP:    base_tile =  4; break;
        case FACE_LEFT:  base_tile =  8; break;
        case FACE_RIGHT: base_tile = 12; break;
        default:         base_tile =  0; break;
    }

    /* Animate between tile 0 and tile 1 within each facing group */
    anim_offset = (u8)((s_anim_frame >> 3) & 1u);
    tile_id     = (u8)(base_tile + anim_offset);

    sprite_set(p->sprite_id, sx, sy, tile_id, 0);
}
