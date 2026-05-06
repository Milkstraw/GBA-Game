#include "inventory.h"
#include "demo_map.h"
#include "player.h"
#include "tile_engine.h"

/* Product catalog in EWRAM */
Product g_products[MAX_PRODUCTS] __attribute__((section(".ewram")));

/* Player cash in cents; guard: never unsigned-underflow below 0 */
u16 g_cash = 0;

/* Total units across all backroom stock */
u8 g_backroom_total = 0;

/*
 * Hardcoded product→shelf mapping for the demo:
 *   Products 0 (Bread) and 1 (Milk) share Shelf 1 (S1, shelf_id=0).
 *   Product 2 (Apples) is on Shelf 2 (S2, shelf_id=1).
 *   Products 3+ have no shelf assigned in demo (shelf_id=0xFF).
 */
static const u8 k_product_shelf[MAX_PRODUCTS] = {
    0, 0, 1,                    /* Bread, Milk → shelf 0; Apples → shelf 1 */
    0xFF,0xFF,0xFF,0xFF,0xFF,   /* slots 3-7 */
    0xFF,0xFF,0xFF,0xFF,0xFF,   /* slots 8-12 */
    0xFF,0xFF,0xFF,0xFF,0xFF,   /* slots 13-17 */
    0xFF,0xFF,0xFF,0xFF,0xFF,   /* slots 18-22 */
    0xFF,0xFF,0xFF,0xFF,0xFF,   /* slots 23-27 */
    0xFF,0xFF,0xFF,0xFF         /* slots 28-31 */
};

void inventory_init(void)
{
    u8 i;

    /* Clear all product slots */
    for (i = 0; i < MAX_PRODUCTS; i++) {
        g_products[i].id             = i;
        g_products[i].name[0]        = '\0';
        g_products[i].category       = CAT_PRODUCE;
        g_products[i].shelf_price    = 0;
        g_products[i].cost_price     = 0;
        g_products[i].market_price   = 0;
        g_products[i].stock_on_shelf = 0;
        g_products[i].shelf_capacity = 0;
        g_products[i].stock_in_back  = 0;
        g_products[i].units_sold_today = 0;
    }

    /* Demo product 0 — Bread */
    g_products[0].id             = 0;
    g_products[0].name[0] = 'B'; g_products[0].name[1] = 'r';
    g_products[0].name[2] = 'e'; g_products[0].name[3] = 'a';
    g_products[0].name[4] = 'd'; g_products[0].name[5] = '\0';
    g_products[0].category       = CAT_BAKERY;
    g_products[0].cost_price     = 80;
    g_products[0].shelf_price    = 149;
    g_products[0].market_price   = 149;
    g_products[0].shelf_capacity = 6;
    g_products[0].stock_on_shelf = 0;
    g_products[0].stock_in_back  = 0;

    /* Demo product 1 — Milk */
    g_products[1].id             = 1;
    g_products[1].name[0] = 'M'; g_products[1].name[1] = 'i';
    g_products[1].name[2] = 'l'; g_products[1].name[3] = 'k';
    g_products[1].name[4] = '\0';
    g_products[1].category       = CAT_DAIRY;
    g_products[1].cost_price     = 110;
    g_products[1].shelf_price    = 229;
    g_products[1].market_price   = 229;
    g_products[1].shelf_capacity = 4;
    g_products[1].stock_on_shelf = 0;
    g_products[1].stock_in_back  = 0;

    /* Demo product 2 — Apples */
    g_products[2].id             = 2;
    g_products[2].name[0] = 'A'; g_products[2].name[1] = 'p';
    g_products[2].name[2] = 'p'; g_products[2].name[3] = 'l';
    g_products[2].name[4] = 'e'; g_products[2].name[5] = 's';
    g_products[2].name[6] = '\0';
    g_products[2].category       = CAT_PRODUCE;
    g_products[2].cost_price     = 50;
    g_products[2].shelf_price    = 99;
    g_products[2].market_price   = 99;
    g_products[2].shelf_capacity = 8;
    g_products[2].stock_on_shelf = 0;
    g_products[2].stock_in_back  = 0;

    g_cash           = STARTING_CASH_CENTS;
    g_backroom_total = 0;
}

u8 inventory_shelf_for_product(u8 product_id)
{
    if (product_id >= MAX_PRODUCTS) return 0xFF;
    return k_product_shelf[product_id];
}

u8 inventory_order(u8 product_id, u8 qty)
{
    u16 total_cost;
    Product *p;

    if (product_id >= MAX_PRODUCTS || qty == 0) return 0;

    p          = &g_products[product_id];
    total_cost = (u16)p->cost_price * qty;

    /* Guard: insufficient cash */
    if (g_cash < total_cost) return 0;

    g_cash -= total_cost;
    p->stock_in_back  = (u8)(p->stock_in_back  + qty);
    g_backroom_total  = (u8)(g_backroom_total   + qty);

    return 1;
}

void inventory_stock_shelf(u8 product_id)
{
    Product *p;
    u8 was_empty;

    if (product_id >= MAX_PRODUCTS) return;

    p = &g_products[product_id];

    if (p->stock_in_back == 0)            return;  /* nothing to pull */
    if (p->stock_on_shelf >= p->shelf_capacity) return;  /* shelf full */

    was_empty = (p->stock_on_shelf == 0);

    p->stock_in_back--;
    p->stock_on_shelf++;
    if (g_backroom_total > 0) g_backroom_total--;

    /* Update shelf tile when transitioning empty → stocked */
    if (was_empty && p->stock_on_shelf > 0) {
        u8 shelf_id = inventory_shelf_for_product(product_id);
        if (shelf_id != 0xFF) {
            demo_map_set_shelf_state(shelf_id, 1);
        }
    }
}

u8 inventory_sell(u8 product_id)
{
    Product *p;
    u8 now_empty;

    if (product_id >= MAX_PRODUCTS) return 0;

    p = &g_products[product_id];

    if (p->stock_on_shelf == 0) return 0;  /* shelf empty */

    p->stock_on_shelf--;
    g_cash += p->shelf_price;
    p->units_sold_today++;

    /* Update shelf tile when transitioning stocked → empty.
     * Multiple products can share a shelf; only mark empty when ALL
     * products on that shelf have zero stock_on_shelf. */
    now_empty = (p->stock_on_shelf == 0);
    if (now_empty) {
        u8 shelf_id = inventory_shelf_for_product(product_id);
        if (shelf_id != 0xFF) {
            u8 j, any_stocked = 0;
            for (j = 0; j < MAX_PRODUCTS; j++) {
                if (k_product_shelf[j] == shelf_id &&
                    g_products[j].stock_on_shelf > 0) {
                    any_stocked = 1;
                    break;
                }
            }
            if (!any_stocked) {
                demo_map_set_shelf_state(shelf_id, 0);
            }
        }
    }

    return 1;
}

/* ----------------------------------------------------------------
 * player_on_interact override — shelf stocking
 *
 * Called when the player presses A facing a tile with COL_INTERACT set.
 * If the tile also has COL_SHELF, stock the shelf products from backroom.
 * ---------------------------------------------------------------- */
void player_on_interact(u8 tile_x, u8 tile_y)
{
    u8 tile, shelf_id, j;

    tile = map_tile_at(tile_x, tile_y);
    if (!(g_collision_flags[tile] & COL_SHELF)) return;

    /* Determine which shelf based on position */
    if (tile_y == 2 && tile_x >= 3 && tile_x <= 5)      shelf_id = 0;  /* S1 */
    else if (tile_y == 2 && tile_x >= 8 && tile_x <= 10) shelf_id = 1;  /* S2 */
    else if (tile_y == 5 && tile_x >= 3 && tile_x <= 5)  shelf_id = 2;  /* S3 */
    else return;

    /* Stock all products assigned to this shelf */
    for (j = 0; j < MAX_PRODUCTS; j++) {
        if (k_product_shelf[j] == shelf_id) {
            inventory_stock_shelf(j);
        }
    }
}
