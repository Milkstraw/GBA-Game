#pragma once
#include "../core/types.h"
#include "../core/clock.h"
#include "../systems/weather.h"

void load_bg_palette(const u16 *pal, u8 pal_index);
void load_sprite_palette(const u16 *pal, u8 pal_index);
void palette_apply_time_of_day(DayPhase phase, u8 hour);
void palette_apply_weather(WeatherState weather);
