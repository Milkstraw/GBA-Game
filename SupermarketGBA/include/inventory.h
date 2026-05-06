#ifndef INVENTORY_H
#define INVENTORY_H

#include "gba_types.h"
#include "constants.h"

/* Product category enum */
typedef enum {
    CAT_PRODUCE  = 0,
    CAT_DAIRY    = 1,
    CAT_BAKERY   = 2,
    CAT_MEAT     = 3,
    CAT_FROZEN   = 4,
    CAT_DRY      = 5,
    CAT_BEVERAGE = 6,
    CAT_HEALTH   = 7
} ProductCategory;

/* Product data (matches GDD spec exactly) */
typedef struct {
    u8   id;
    char name[13];        /* 12 chars + null terminator */
    u8   category;        /* ProductCategory enum value */
    u16  shelf_price;     /* cents, e.g. 149 = $1.49 */
    u16  cost_price;      /* what player pays to order */
    u16  market_price;    /* fluctuates Phase 2+ */
    u8   stock_on_shelf;
    u8   shelf_capacity;
    u8   stock_in_back;
    u8   units_sold_today;
} Product;

/* Product catalog — placed in EWRAM (256KB slow RAM) */
extern Product g_products[MAX_PRODUCTS] __attribute__((section(".ewram")));

/* Player's cash balance in cents; starts at STARTING_CASH_CENTS (5000) */
extern u16 g_cash;

/* Total units currently in the backroom (all products combined) */
extern u8 g_backroom_total;

/* ----------------------------------------------------------------
 * API
 * ---------------------------------------------------------------- */

/* Populate g_products[0..2] with demo data, set g_cash = STARTING_CASH_CENTS. */
void inventory_init(void);

/*
 * Place an order for qty units of product_id.
 * Deducts cost_price*qty from g_cash immediately (demo: instant delivery).
 * Adds qty to stock_in_back and g_backroom_total.
 * Returns 1 on success, 0 if insufficient cash.
 */
u8 inventory_order(u8 product_id, u8 qty);

/*
 * Move one unit from back-stock to shelf for product_id.
 * Respects shelf_capacity — does nothing if shelf is full.
 * Updates g_backroom_total.  If this causes the shelf to transition from
 * empty to stocked, calls demo_map_set_shelf_state.
 */
void inventory_stock_shelf(u8 product_id);

/*
 * Sell one unit of product_id (NPC or checkout).
 * Decrements stock_on_shelf, adds shelf_price to g_cash, increments
 * units_sold_today.  If shelf becomes empty, calls demo_map_set_shelf_state.
 * Returns 1 on success, 0 if no shelf stock.
 */
u8 inventory_sell(u8 product_id);

/* Map product id → shelf id (0=S1, 1=S2, 2=S3, 0xFF=unassigned).
 * Used internally and by demo_map_set_shelf_state calls. */
u8 inventory_shelf_for_product(u8 product_id);

#endif /* INVENTORY_H */
