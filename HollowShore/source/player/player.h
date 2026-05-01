#pragma once
#include "../core/types.h"
#include "stats.h"

typedef enum {
    FACE_UP = 0,
    FACE_DOWN,
    FACE_LEFT,
    FACE_RIGHT
} Facing;

typedef enum {
    PSTATE_IDLE = 0,
    PSTATE_MOVING,
    PSTATE_ATTACKING,
    PSTATE_INTERACTING
} PlayerState;

typedef struct {
    s16         x;          /* world pixel x */
    s16         y;          /* world pixel y */
    Facing      facing;
    PlayerState state;
    PlayerStats stats;
    u8          sprite_id;
} Player;

void player_init(Player *p);
void player_update(Player *p, u16 keys);
void player_draw(const Player *p, s16 cam_x, s16 cam_y);
