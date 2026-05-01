#pragma once
#include "../core/types.h"
#include "../player/inventory.h"
#include "../systems/crafting.h"

void draw_inventory_screen(const Inventory *inv);
void draw_crafting_screen(const Inventory *inv);
void draw_pause_menu(void);
void draw_world_select_screen(void);
