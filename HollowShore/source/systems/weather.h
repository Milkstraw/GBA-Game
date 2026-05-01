#pragma once
#include "../core/types.h"
#include "farming.h"

typedef enum {
    WEATHER_CLEAR = 0,
    WEATHER_RAIN,
    WEATHER_STORM,
    WEATHER_BLIZZARD
} WeatherState;

extern WeatherState current_weather;

void         weather_update(u16 day, Season season);
WeatherState weather_get(void);
void         weather_apply_effects(void);  /* called each frame */
