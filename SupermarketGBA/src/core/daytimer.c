#include "daytimer.h"
#include "inventory.h"

DayTimer g_daytimer;

void daytimer_init(void)
{
    g_daytimer.minute         = 0;
    g_daytimer.hour           = 8;
    g_daytimer.minute_of_hour = 0;
    g_daytimer.day            = 1;
    g_daytimer.store_open     = 1;
    g_daytimer.frame_accum    = 0;
}

void daytimer_update(void)
{
    if (!g_daytimer.store_open) return;

    g_daytimer.frame_accum++;
    if (g_daytimer.frame_accum < DAYTIMER_FRAMES_PER_MIN) return;

    g_daytimer.frame_accum = 0;
    g_daytimer.minute++;

    /* Update derived hour/minute fields */
    g_daytimer.hour           = (u8)(8u + g_daytimer.minute / 60u);
    g_daytimer.minute_of_hour = (u8)(g_daytimer.minute % 60u);

    /* Check for store close */
    if (g_daytimer.minute >= DAYTIMER_CLOSE_MIN) {
        daytimer_end_of_day();
    }
}

/* Weak definition; save.c or main.c provides the full version that also
 * saves before resetting.  This stub handles the timer state only. */
__attribute__((weak)) void daytimer_end_of_day(void)
{
    u8 i;

    /* Reset daily sales on all products */
    for (i = 0; i < MAX_PRODUCTS; i++) {
        g_products[i].units_sold_today = 0;
    }

    g_daytimer.day++;
    g_daytimer.minute         = 0;
    g_daytimer.hour           = 8;
    g_daytimer.minute_of_hour = 0;
    g_daytimer.frame_accum    = 0;
    g_daytimer.store_open     = 1;
}

void daytimer_get_time_str(char *buf)
{
    /* Format: "HH:MM AP\0" — 8 visible chars + null = 9 bytes */
    u8 h   = g_daytimer.hour;
    u8 m   = g_daytimer.minute_of_hour;
    u8 pm  = (h >= 12) ? 1u : 0u;
    u8 h12 = (u8)(h % 12u);
    if (h12 == 0) h12 = 12;

    buf[0] = (char)('0' + h12 / 10u);
    buf[1] = (char)('0' + h12 % 10u);
    buf[2] = ':';
    buf[3] = (char)('0' + m / 10u);
    buf[4] = (char)('0' + m % 10u);
    buf[5] = ' ';
    buf[6] = pm ? 'P' : 'A';
    buf[7] = 'M';
    buf[8] = '\0';
}
