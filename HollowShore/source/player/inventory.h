#pragma once
#include "../core/types.h"

#define INVENTORY_SLOTS  24  /* 16 base + 8 hotbar */
#define HOTBAR_SLOTS      8

typedef enum {
    ITEM_NONE = 0,
    /* Tools */
    ITEM_AXE, ITEM_PICKAXE, ITEM_SHOVEL, ITEM_FISHING_ROD, ITEM_BOW,
    /* Weapons */
    ITEM_SWORD, ITEM_SPEAR,
    /* Materials */
    ITEM_WOOD, ITEM_STONE, ITEM_FLINT, ITEM_BRANCH, ITEM_FIBER,
    ITEM_IRON_ORE, ITEM_IRON_BAR, ITEM_LEATHER, ITEM_BONE, ITEM_MEAT,
    ITEM_FISH, ITEM_HERB, ITEM_BERRY, ITEM_SEED_BASIC,
    /* Structures */
    ITEM_CAMPFIRE, ITEM_WALL_WOOD, ITEM_WALL_STONE, ITEM_TORCH,
    ITEM_BED, ITEM_WORKBENCH, ITEM_FORGE, ITEM_ALCHEMY_TABLE,
    /* Portal pieces */
    ITEM_VERDANT_SHARD, ITEM_TIDE_SHARD, ITEM_FROST_SHARD,
    ITEM_VEIL_SHARD, ITEM_EMBER_SHARD, ITEM_HOLLOW_KEYSTONE,
    ITEM_TIDE_PORTAL,
    ITEM_COUNT
} ItemType;

typedef struct {
    ItemType type;
    u8       quantity;
} Item;

typedef struct {
    Item slots[INVENTORY_SLOTS];
    u8   hotbar_cursor;  /* 0–7 */
} Inventory;

void inventory_init(Inventory *inv);
bool inventory_add(Inventory *inv, ItemType type, u8 qty);
bool inventory_remove(Inventory *inv, ItemType type, u8 qty);
void inventory_sort(Inventory *inv);
Item *inventory_hotbar_active(Inventory *inv);
