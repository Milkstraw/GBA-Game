#pragma once
#include "../core/types.h"

typedef enum {
    DIR_UP = 0,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef enum {
    TRANS_IDLE = 0,
    TRANS_FADING_OUT,
    TRANS_LOADING,
    TRANS_FADING_IN
} TransitionState;

void transition_check(s16 player_x, s16 player_y, u8 current_section_id);
void transition_update(void);
TransitionState transition_get_state(void);
