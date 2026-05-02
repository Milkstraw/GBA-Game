#include "clock.h"
#include "debug.h"

/* 60 frames per in-game minute, 60 minutes per hour, 24 hours per day */
#define FRAMES_PER_MINUTE  60
#define MINUTES_PER_HOUR   60
#define HOURS_PER_DAY      24
#define FRAMES_PER_HOUR    (FRAMES_PER_MINUTE * MINUTES_PER_HOUR)

void clock_init(GameClock *c) {
    c->hour          = 8;   /* start at 8am (DAY) */
    c->day           = 0;
    c->frame_counter = 0;
}

void clock_update(GameClock *c) {
    c->frame_counter++;

    /* Every FRAMES_PER_HOUR frames, advance by one hour */
    if (c->frame_counter >= FRAMES_PER_HOUR) {
        c->frame_counter = 0;
        c->hour++;
        if (c->hour >= HOURS_PER_DAY) {
            c->hour = 0;
            c->day++;
            debug_log_int(LOG_INFO, "clock: day", (s32)c->day);
        }
        debug_log_int(LOG_DEBUG, "clock: hour", (s32)c->hour);
    }
}

DayPhase clock_get_phase(const GameClock *c) {
    u8 h = c->hour;
    if (h >= 5 && h <= 7)  return PHASE_DAWN;
    if (h >= 8 && h <= 17) return PHASE_DAY;
    if (h >= 18 && h <= 20) return PHASE_DUSK;
    return PHASE_NIGHT;
}
