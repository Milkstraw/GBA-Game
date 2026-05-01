#pragma once
#include "../core/types.h"
#include "../player/inventory.h"

#define MAX_RECIPE_INGREDIENTS  4
#define MAX_RECIPES            64

typedef enum {
    STATION_HANDS = 0,
    STATION_WORKBENCH,
    STATION_FORGE,
    STATION_COOKING_FIRE,
    STATION_ALCHEMY_TABLE
} CraftStation;

typedef struct {
    ItemType     ingredient[MAX_RECIPE_INGREDIENTS];
    u8           qty[MAX_RECIPE_INGREDIENTS];
    u8           num_ingredients;
    ItemType     result;
    u8           result_qty;
    CraftStation station;
    bool         unlocked;
} Recipe;

extern Recipe recipe_book[MAX_RECIPES];
extern u8     recipe_count;

void crafting_init(void);
void crafting_unlock_check(ItemType picked_up);
bool crafting_execute(u8 recipe_id, Inventory *inv);
