#pragma once
#include "../core/types.h"
#include "../player/stats.h"
#include "../player/inventory.h"

void hud_draw(const PlayerStats *stats, const Inventory *inv);
void draw_bar(u8 x, u8 y, s16 value, s16 max, u16 color);
