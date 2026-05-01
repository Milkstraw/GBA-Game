#pragma once
#include "types.h"

typedef enum {
    PHASE_DAWN,
    PHASE_DAY,
    PHASE_DUSK,
    PHASE_NIGHT
} DayPhase;

typedef struct {
    u8  hour;           /* 0–23 in-game hours */
    u16 day;            /* total days elapsed */
    u32 frame_counter;  /* raw frame accumulator */
} GameClock;

void     clock_init(GameClock *c);
void     clock_update(GameClock *c);
DayPhase clock_get_phase(const GameClock *c);
