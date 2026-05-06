#ifndef DAYTIMER_H
#define DAYTIMER_H

#include "gba_types.h"

/*
 * GBA runs at ~59.727 fps.  We use 4 frames per in-game minute so that:
 *   1 in-game hour  =  60 min × 4 frames =  240 frames
 *   1 in-game day   = 720 min × 4 frames = 2880 frames  (12-hr store day)
 *
 * Store open:  minute 0   = 8:00 AM
 * Store close: minute 720 = 8:00 PM
 */
#define DAYTIMER_FRAMES_PER_MIN  4u
#define DAYTIMER_OPEN_MIN        0u
#define DAYTIMER_CLOSE_MIN       720u

typedef struct {
    u16 minute;         /* 0–719: in-game minutes since 8 AM */
    u8  hour;           /* 8–19: real hour (8 + minute/60) */
    u8  minute_of_hour; /* 0–59: minute within hour */
    u8  day;            /* starts at 1; increments each end-of-day */
    u8  store_open;     /* 1 = store is open */
    u8  frame_accum;    /* counts up to DAYTIMER_FRAMES_PER_MIN then ticks */
} DayTimer;

extern DayTimer g_daytimer;

/* Set minute=0, day=1, store_open=1, clear frame_accum. */
void daytimer_init(void);

/*
 * Advance the timer by one frame.  Ticks minute when frame_accum reaches
 * DAYTIMER_FRAMES_PER_MIN.  Calls daytimer_end_of_day() when minute >= 720.
 */
void daytimer_update(void);

/*
 * Called automatically when minute reaches 720 (8 PM).
 * Resets daily product stats, increments day, resets minute to 0.
 * Calls save_write(0) then sets store_open=1 for the next day.
 * Declared weak so save.c can override or main.c can hook it.
 */
void daytimer_end_of_day(void);

/*
 * Write a 12-hour formatted time string into buf.
 * Format: "HH:MM AM" or "HH:MM PM" — always 8 chars + null (9 total).
 * buf must be at least 9 bytes.
 */
void daytimer_get_time_str(char *buf);

#endif /* DAYTIMER_H */
