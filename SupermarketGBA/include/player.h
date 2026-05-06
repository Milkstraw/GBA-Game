#ifndef PLAYER_H
#define PLAYER_H

#include "gba_types.h"

/* Direction constants used by Player.dir */
#define DIR_DOWN  0
#define DIR_UP    1
#define DIR_LEFT  2
#define DIR_RIGHT 3

typedef struct {
    u8 x, y;         /* tile position (0-based column/row) */
    u8 px, py;       /* pixel position: x*16, y*16 */
    u8 dir;          /* DIR_DOWN/UP/LEFT/RIGHT */
    u8 anim_frame;   /* 0, 1, 2 — walk cycle index */
    u8 anim_timer;   /* frame counter; advances anim_frame every 8 frames */
    u8 is_moving;    /* 1 if a d-pad direction is held this frame */
} Player;

extern Player g_player;

/* Place player at tile (6,6), clear OAM slots 0-1. */
void player_init(void);

/* Read input, check collision, move, update animation, handle A-interact. */
void player_update(void);

/* Write OAM entries 0 (16×16) and 1 (16×8) with current position + tile. */
void player_draw(void);

/* Called when the player presses A facing an interact-flagged tile.
 * Declared __attribute__((weak)) so other systems can override it. */
void player_on_interact(u8 tile_x, u8 tile_y);

#endif /* PLAYER_H */
