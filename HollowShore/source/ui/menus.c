/*
 * menus.c — Inventory, crafting, pause, and world-select screen rendering.
 *
 * All drawing goes to BG3 via text_draw() / text_draw_int().
 * No dynamic allocation; all layout is fixed tile-coordinate arithmetic.
 *
 * Screen dimensions: 30 columns x 20 rows of 8x8 tiles.
 *
 * Inventory layout:
 *   Row  1: "INVENTORY" header
 *   Rows 3-10: 4x4 grid of main slots (16 slots, indices 0-15)
 *   Row 17: 8-slot hotbar row (indices 16-23)
 *
 * Crafting layout:
 *   Row  1: "CRAFTING" header
 *   Rows 3-17: list of unlocked recipes (up to 15 visible)
 *
 * Pause menu layout:
 *   Row  7: "-- PAUSED --"
 *   Row 10: "RESUME"
 *   Row 12: "SAVE"
 *   Row 14: "QUIT"
 *
 * World select layout:
 *   Row  3: "SELECT WORLD"
 *   Row  6: world options
 */

#include "../gba.h"
#include "menus.h"
#include "text.h"
#include "../player/inventory.h"
#include "../systems/crafting.h"

/* -----------------------------------------------------------------------
 * Item name table (matches ItemType enum)
 * -------------------------------------------------------------------- */
static const char * const menu_item_names[] = {
    "None",
    "Axe",
    "Pickaxe",
    "Shovel",
    "Rod",
    "Bow",
    "Sword",
    "Spear",
    "Wood",
    "Stone",
    "Flint",
    "Branch",
    "Fiber",
    "Iron Ore",
    "Iron Bar",
    "Leather",
    "Bone",
    "Meat",
    "Fish",
    "Herb",
    "Berry",
    "Seed",
    "Campfire",
    "Wd Wall",
    "St Wall",
    "Torch",
    "Bed",
    "Wrkbnch",
    "Forge",
    "Alch Tbl",
    "V.Shard",
    "T.Shard",
    "F.Shard",
    "Vl.Shard",
    "Em.Shard",
    "Keystone",
    "Td Portal",
};

#define MENU_ITEM_NAME_COUNT ((u8)(sizeof(menu_item_names) / sizeof(menu_item_names[0])))

/* -----------------------------------------------------------------------
 * Internal helper: safe item name lookup
 * -------------------------------------------------------------------- */
static const char *item_name_safe(ItemType type)
{
    if ((u8)type < MENU_ITEM_NAME_COUNT) {
        return menu_item_names[(u8)type];
    }
    return "???";
}

/* -----------------------------------------------------------------------
 * Internal helper: write a decimal u8 value without text_draw_int (avoids
 * s32 path for small unsigned quantities).
 * -------------------------------------------------------------------- */
static void draw_qty(u8 x, u8 y, u8 val)
{
    char buf[4]; /* "255\0" */
    u8   pos = 3u;

    buf[pos] = '\0';
    if (val == 0u) {
        pos--;
        buf[pos] = '0';
    } else {
        u8 v = val;
        while (v > 0u) {
            pos--;
            buf[pos] = (char)('0' + (v % 10u));
            v /= 10u;
        }
    }
    text_draw(x, y, &buf[pos]);
}

/* -----------------------------------------------------------------------
 * Internal: check whether the inventory contains all ingredients for
 * recipe at index rid. Returns TRUE if craftable.
 * -------------------------------------------------------------------- */
static bool recipe_craftable(const Inventory *inv, u8 rid)
{
    const Recipe *r = &recipe_book[rid];
    u8 ing;

    for (ing = 0u; ing < r->num_ingredients; ing++) {
        ItemType need_type = r->ingredient[ing];
        u8       need_qty  = r->qty[ing];
        u8       have_qty  = 0u;
        u8       s;

        for (s = 0u; s < INVENTORY_SLOTS; s++) {
            if (inv->slots[s].type == need_type) {
                have_qty += inv->slots[s].quantity;
            }
        }

        if (have_qty < need_qty) {
            return FALSE;
        }
    }
    return TRUE;
}

/* -----------------------------------------------------------------------
 * draw_inventory_screen
 *
 * Slots 0-15  = main inventory (4x4 grid)
 * Slots 16-23 = hotbar (single row)
 *
 * Each cell shows an abbreviated item name (first 5 chars) and quantity.
 * Cell width = 7 tiles (5 name + 'x' + qty digit).
 * Grid columns: 0,7,14,21  (4 columns of 7 tiles each)
 * Grid rows: start at row 3, step 2 rows per entry (4 rows x 2 = 8, rows 3-10)
 * -------------------------------------------------------------------- */
void draw_inventory_screen(const Inventory *inv)
{
    u8 i;
    static const u8 MAIN_SLOTS = 16u;

    text_clear();
    text_draw(10u, 1u, "INVENTORY");
    text_draw(0u,  2u, "-----------------------------");

    /* Main inventory grid: 4 columns x 4 rows */
    for (i = 0u; i < MAIN_SLOTS; i++) {
        u8 col = (u8)(i % 4u);
        u8 row = (u8)(i / 4u);
        u8 tx  = (u8)(col * 7u + 1u);   /* x: 1, 8, 15, 22 */
        u8 ty  = (u8)(row * 2u + 3u);   /* y: 3, 5, 7, 9   */

        const Item *it = &inv->slots[i];

        if (it->type != ITEM_NONE) {
            const char *name = item_name_safe(it->type);
            /* Print up to 5 chars of name */
            char abbr[6];
            u8   k;
            for (k = 0u; k < 5u; k++) {
                if (name[k] == '\0') {
                    abbr[k] = ' ';
                } else {
                    abbr[k] = name[k];
                }
            }
            abbr[5] = '\0';
            text_draw(tx, ty, abbr);
            text_draw((u8)(tx + 5u), ty, "x");
            draw_qty((u8)(tx + 6u), ty, it->quantity);
        } else {
            text_draw(tx, ty, "[   ]");
        }
    }

    /* Separator */
    text_draw(0u, 15u, "-----------------------------");
    text_draw(0u, 16u, "HOTBAR:");

    /* Hotbar row: slots 16-23 */
    for (i = 0u; i < HOTBAR_SLOTS; i++) {
        u8 slot = (u8)(MAIN_SLOTS + i);
        u8 tx   = (u8)(i * 3u + 1u);   /* columns 1,4,7,10,13,16,19,22 */
        const Item *it = &inv->slots[slot];

        /* Active slot marker */
        if (i == inv->hotbar_cursor) {
            text_draw(tx, 17u, "[");
        }

        if (it->type != ITEM_NONE) {
            /* First letter of item name */
            char fl[2];
            fl[0] = item_name_safe(it->type)[0];
            fl[1] = '\0';
            text_draw((u8)(tx + (i == inv->hotbar_cursor ? 1u : 0u)), 17u, fl);
        } else {
            text_draw((u8)(tx + (i == inv->hotbar_cursor ? 1u : 0u)), 17u, "-");
        }

        if (i == inv->hotbar_cursor) {
            text_draw((u8)(tx + 2u), 17u, "]");
        }
    }
}

/* -----------------------------------------------------------------------
 * draw_crafting_screen
 *
 * Lists all unlocked recipes. A '>' prefix marks recipes where the player
 * has all required ingredients. Shows up to 15 recipes on screen.
 *
 * Format per line: "> ItemName (NxIngr, ...)"
 * -------------------------------------------------------------------- */
void draw_crafting_screen(const Inventory *inv)
{
    u8  rid;
    u8  row = 3u;
    u8  count = 0u;
    static const u8 MAX_VISIBLE = 15u;

    text_clear();
    text_draw(11u, 1u, "CRAFTING");
    text_draw(0u,  2u, "-----------------------------");

    for (rid = 0u; rid < recipe_count && count < MAX_VISIBLE; rid++) {
        const Recipe *r = &recipe_book[rid];

        if (!r->unlocked) {
            continue;
        }

        /* Craftable marker */
        if (recipe_craftable(inv, rid)) {
            text_draw(0u, row, ">");
        } else {
            text_draw(0u, row, " ");
        }

        /* Result item name */
        text_draw(2u, row, item_name_safe(r->result));

        /* Ingredient summary: "Nx<name>" for each ingredient */
        {
            u8  ing;
            u8  ix = 12u;  /* start after item name (approx col 12) */

            for (ing = 0u; ing < r->num_ingredients && ix < 28u; ing++) {
                if (ing > 0u) {
                    text_draw(ix, row, ",");
                    ix++;
                }
                draw_qty(ix, row, r->qty[ing]);
                ix++;
                text_draw(ix, row, "x");
                ix++;
                /* First 3 chars of ingredient name */
                {
                    const char *iname = item_name_safe(r->ingredient[ing]);
                    char abbr[4];
                    u8   k;
                    for (k = 0u; k < 3u && iname[k] != '\0' && ix + k < 29u; k++) {
                        abbr[k] = iname[k];
                    }
                    abbr[k] = '\0';
                    text_draw(ix, row, abbr);
                    ix += k;
                }
            }
        }

        row++;
        count++;
    }

    if (count == 0u) {
        text_draw(2u, 4u, "No recipes unlocked.");
    }
}

/* -----------------------------------------------------------------------
 * draw_pause_menu
 * -------------------------------------------------------------------- */
void draw_pause_menu(void)
{
    text_clear();

    /* Decorative border */
    text_draw(8u,  5u, "==============");
    text_draw(8u,  6u, "  -- PAUSED --");
    text_draw(8u,  7u, "==============");

    /* Menu options */
    text_draw(12u, 10u, "RESUME");
    text_draw(12u, 12u, "SAVE");
    text_draw(12u, 14u, "QUIT");

    /* Cursor hint */
    text_draw(10u, 10u, ">");  /* default selection on RESUME */
}

/* -----------------------------------------------------------------------
 * draw_world_select_screen
 * -------------------------------------------------------------------- */
void draw_world_select_screen(void)
{
    text_clear();

    text_draw(9u,  1u, "== SELECT WORLD ==");
    text_draw(0u,  2u, "-----------------------------");

    text_draw(4u,  5u, "1. The Verdant Expanse");
    text_draw(4u,  8u, "2. Shattered Isles");
    text_draw(4u, 11u, "3. The Hollow Depths");

    text_draw(0u, 15u, "-----------------------------");
    text_draw(3u, 17u, "Press A to select a world.");
}
