/*
 * weather.c — Season-weighted weather simulation for HollowShore
 * Wave 5b implementation
 */

#include "weather.h"
#include "farming.h"
#include "../graphics/palette.h"
#include "../core/types.h"

/* Current weather state — extern declared in weather.h */
WeatherState current_weather = WEATHER_CLEAR;

/* LCG pseudo-random number generator */
static u32 rng = 12345u;

static u32 lcg_next(void)
{
    rng = rng * 1664525u + 1013904223u;
    return rng;
}

/* Frame counter for period-based effects */
static u16 frame_counter = 0;

/*
 * Crop grid dimensions for auto-watering.
 * These match the world section tile grid (160x160).
 * Rain applies to every tile in the grid; farming_water ignores non-tilled
 * tiles internally, so iterating the full grid is safe.
 */
#define CROP_GRID_W  160
#define CROP_GRID_H  160

/* -------------------------------------------------------------------------
 * weather_update
 * Call once per in-game day.  Uses a simple LCG with season-weighted thresholds
 * to select the new weather state.
 * -------------------------------------------------------------------------*/
void weather_update(u16 day, Season season)
{
    u32 roll;
    u16 threshold;

    /* Mix day into RNG for extra variety */
    rng ^= (u32)day;
    roll = lcg_next();

    /* Map to 0–99 */
    threshold = (u16)(roll % 100u);

    switch (season)
    {
        case SEASON_SPRING:
            /*
             * 40% CLEAR  (0–39)
             * 40% RAIN   (40–79)
             * 20% STORM  (80–99)
             */
            if (threshold < 40)
                current_weather = WEATHER_CLEAR;
            else if (threshold < 80)
                current_weather = WEATHER_RAIN;
            else
                current_weather = WEATHER_STORM;
            break;

        case SEASON_SUMMER:
            /*
             * 60% CLEAR  (0–59)
             * 20% RAIN   (60–79)
             * 20% STORM  (80–99)
             */
            if (threshold < 60)
                current_weather = WEATHER_CLEAR;
            else if (threshold < 80)
                current_weather = WEATHER_RAIN;
            else
                current_weather = WEATHER_STORM;
            break;

        case SEASON_AUTUMN:
            /*
             * 30% CLEAR    (0–29)
             * 40% RAIN     (30–69)
             * 20% STORM    (70–89)
             * 10% BLIZZARD (90–99)
             */
            if (threshold < 30)
                current_weather = WEATHER_CLEAR;
            else if (threshold < 70)
                current_weather = WEATHER_RAIN;
            else if (threshold < 90)
                current_weather = WEATHER_STORM;
            else
                current_weather = WEATHER_BLIZZARD;
            break;

        case SEASON_WINTER:
        default:
            /*
             * 20% CLEAR    (0–19)
             * 20% RAIN     (20–39)
             * 60% BLIZZARD (40–99)
             */
            if (threshold < 20)
                current_weather = WEATHER_CLEAR;
            else if (threshold < 40)
                current_weather = WEATHER_RAIN;
            else
                current_weather = WEATHER_BLIZZARD;
            break;
    }
}

/* -------------------------------------------------------------------------
 * weather_get
 * -------------------------------------------------------------------------*/
WeatherState weather_get(void)
{
    return current_weather;
}

/* -------------------------------------------------------------------------
 * weather_apply_effects
 * Called once per frame from the main loop.
 *
 * RAIN     — auto-water all crop tiles every 300 frames
 * STORM    — same as RAIN + dim palette via palette_apply_weather
 * BLIZZARD — same as STORM  (player speed penalty applied by caller via
 *            weather_get())
 * -------------------------------------------------------------------------*/
void weather_apply_effects(void)
{
    ++frame_counter;

    switch (current_weather)
    {
        case WEATHER_CLEAR:
            /* Nothing to do */
            break;

        case WEATHER_RAIN:
        {
            /* Auto-water crops every 300 frames */
            if (frame_counter >= 300)
            {
                u8 x, y;
                frame_counter = 0;
                for (y = 0; y < CROP_GRID_H; ++y)
                {
                    for (x = 0; x < CROP_GRID_W; ++x)
                    {
                        farming_water(x, y, 0); /* day=0 as placeholder; farming.c stores it */
                    }
                }
            }
            break;
        }

        case WEATHER_STORM:
        {
            /* Auto-water crops every 300 frames */
            if (frame_counter >= 300)
            {
                u8 x, y;
                frame_counter = 0;
                for (y = 0; y < CROP_GRID_H; ++y)
                {
                    for (x = 0; x < CROP_GRID_W; ++x)
                    {
                        farming_water(x, y, 0);
                    }
                }
                /* Dim palette to reflect storm atmosphere */
                palette_apply_weather(WEATHER_STORM);
            }
            break;
        }

        case WEATHER_BLIZZARD:
        {
            /* Same as STORM for auto-watering (snow melts onto crops) */
            if (frame_counter >= 300)
            {
                u8 x, y;
                frame_counter = 0;
                for (y = 0; y < CROP_GRID_H; ++y)
                {
                    for (x = 0; x < CROP_GRID_W; ++x)
                    {
                        farming_water(x, y, 0);
                    }
                }
                /* Apply blizzard palette tint */
                palette_apply_weather(WEATHER_BLIZZARD);
            }
            break;
        }
    }
}
