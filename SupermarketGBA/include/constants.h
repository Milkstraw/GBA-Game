#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "gba_types.h"

/* ----------------------------------------------------------------
 * Screen / tile / map layout
 * ---------------------------------------------------------------- */
#define SCREEN_W   240
#define SCREEN_H   160
#define HUD_H       10   /* top bar height in pixels */
#define PLAY_H     (SCREEN_H - HUD_H)  /* 150px effective play area */

#define TILE_W      16   /* meta-tile width  (4 × 8px hardware tiles) */
#define TILE_H      16   /* meta-tile height (4 × 8px hardware tiles) */
#define MAP_W       15   /* meta-tile columns per screen */
#define MAP_H       10   /* meta-tile rows per screen */

/* ----------------------------------------------------------------
 * SRAM — 8-bit bus; always access one byte at a time
 * ---------------------------------------------------------------- */
#define SRAM_BASE  ((volatile u8*)0x0E000000)

/* ----------------------------------------------------------------
 * Object pool caps (all arrays statically allocated)
 * ---------------------------------------------------------------- */
#define MAX_NPCS      8
#define MAX_SHELVES   32
#define MAX_PRODUCTS  32
#define MAX_ORDERS    8
#define BACKROOM_CAP  30

/* ----------------------------------------------------------------
 * Prices in cents (u16); format: $X.XX → X*100 + cents
 * ---------------------------------------------------------------- */
#define STARTING_CASH_CENTS   5000u   /* $50.00 */

/* Demo product — Bread */
#define PRICE_BREAD_COST      80u     /* $0.80 cost  */
#define PRICE_BREAD_SHELF     149u    /* $1.49 shelf */

/* Demo product — Milk */
#define PRICE_MILK_COST       110u    /* $1.10 cost  */
#define PRICE_MILK_SHELF      229u    /* $2.29 shelf */

/* Demo product — Apples */
#define PRICE_APPLES_COST     50u     /* $0.50 cost  */
#define PRICE_APPLES_SHELF    99u     /* $0.99 shelf */

/* Pricing guardrails (Phase 2) */
#define PRICE_FLOOR_PCT       110u    /* min shelf = 110% of cost price  */
#define PRICE_CEILING_PCT     200u    /* max shelf = 200% of market price */

#endif /* CONSTANTS_H */
