/*
 * hud.c — HUD rendering for HollowShore
 *
 * Layout (tile coordinates, screen = 30x20 tiles):
 *   Row  0: active item name (left), day/time readout (right)
 *   Row 17: hotbar slot indicators
 *   Row 18: HP bar (left), hunger bar (right)
 *   Row 19: stamina bar (left)
 *
 * draw_bar() fills a fixed-width strip of tile characters:
 *   filled cells -> solid block character (ASCII 219, glyph index 187)
 *   empty  cells -> period '.' (ASCII 46, glyph index 14)
 *
 * Because BG3 uses a single palette (palette 0) the 'color' parameter
 * cannot change per-bar at the hardware level — it is accepted for API
 * compatibility and could be used to swap palette entries if needed.
 */

#include "../gba.h"
#include "hud.h"
#include "text.h"
#include "../player/inventory.h"

/* -----------------------------------------------------------------------
 * Item name strings (matches ItemType enum order)
 * -------------------------------------------------------------------- */
static const char * const item_names[] = {
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

/* Number of entries in item_names */
#define ITEM_NAME_COUNT ((u8)(sizeof(item_names) / sizeof(item_names[0])))

/* BAR_WIDTH: number of tile columns for each stat bar */
#define BAR_WIDTH 20

/* Screenblock 30 base (same as text.c uses) */
#define SCREENBLOCK_30  ((vu16*)0x06007800)

/* Tile index helpers:
 *   ASCII 219 = solid block  -> glyph index = 219 - 32 = 187
 *   ASCII  46 = '.'          -> glyph index =  46 - 32 =  14
 *   ASCII  32 = ' '          -> glyph index =   0
 */
#define TILE_BLOCK  ((u16)187u)  /* filled bar segment */
#define TILE_DOT    ((u16)14u)   /* empty bar segment  */
#define TILE_SPACE  ((u16)0u)

/* -----------------------------------------------------------------------
 * Internal: write a single tile to the screenblock at (x, y).
 * -------------------------------------------------------------------- */
static void put_tile(u8 x, u8 y, u16 tile)
{
    if (x < 30u && y < 20u) {
        SCREENBLOCK_30[(u16)(y * 32u + x)] = tile;
    }
}

/* -----------------------------------------------------------------------
 * draw_bar — render a horizontal stat bar of BAR_WIDTH tiles.
 *
 * Fills (value * BAR_WIDTH / max) tiles with TILE_BLOCK,
 * the remainder with TILE_DOT.
 *
 * 'color' is kept for API compatibility; on BG3 all tiles share palette 0.
 * -------------------------------------------------------------------- */
void draw_bar(u8 x, u8 y, s16 value, s16 max, u16 color)
{
    s16 filled;
    u8  col;

    /* Suppress unused-parameter warning */
    (void)color;

    if (max <= 0) {
        filled = 0;
    } else if (value <= 0) {
        filled = 0;
    } else if (value >= max) {
        filled = (s16)BAR_WIDTH;
    } else {
        filled = (s16)((s32)value * BAR_WIDTH / (s32)max);
    }

    for (col = 0u; col < BAR_WIDTH; col++) {
        u16 tile = (col < (u8)filled) ? TILE_BLOCK : TILE_DOT;
        put_tile((u8)(x + col), y, tile);
    }
}

/* -----------------------------------------------------------------------
 * Internal: write a decimal u16 inline at a fixed position (no buf alloc).
 * Used to display item quantity without calling text_draw_int's signed path.
 * -------------------------------------------------------------------- */
static void draw_u8_at(u8 x, u8 y, u8 val)
{
    char buf[4];  /* "255\0" */
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
 * hud_draw — draw the full HUD each frame.
 *
 * Call after text_clear() when rebuilding the text layer each frame,
 * or maintain persistent tiles if performance requires it.
 * -------------------------------------------------------------------- */
void hud_draw(const PlayerStats *stats, const Inventory *inv)
{
    /* --- Row 0: active item name (left) -------------------------------- */
    {
        const Item *active = inventory_hotbar_active((Inventory *)inv);
        if (active != (Item*)0 && active->type != ITEM_NONE &&
                (u8)active->type < ITEM_NAME_COUNT) {
            text_draw(0u, 0u, item_names[active->type]);
        } else {
            text_draw(0u, 0u, "---");
        }
    }

    /* --- Row 0 right: day readout "Day NNN" at column 22 --------------- */
    /* (Day counter not passed in; display a static label; clock is
     *  integrated at a higher level — emit the label "DAY" and let the
     *  caller overwrite with clock data if desired.)                      */
    text_draw(22u, 0u, "DAY");

    /* --- Row 17: hotbar slot indicators -------------------------------- */
    /* Show item first letter + ':' + qty for each of the 8 hotbar slots. */
    {
        u8 i;
        /* Hotbar occupies slots [16..23] in inv->slots (last HOTBAR_SLOTS) */
        u8 hotbar_start = (u8)(INVENTORY_SLOTS - HOTBAR_SLOTS);
        for (i = 0u; i < HOTBAR_SLOTS; i++) {
            u8 slot = (u8)(hotbar_start + i);
            u8 col  = (u8)(i * 3u + 1u);  /* columns 1,4,7,10,13,16,19,22 */
            const Item *it = &inv->slots[slot];

            if (it->type != ITEM_NONE && (u8)it->type < ITEM_NAME_COUNT) {
                char first = item_names[it->type][0];
                /* Active slot indicator */
                if (i == inv->hotbar_cursor) {
                    put_tile((u8)(col - 1u), 17u, (u16)('^' - 32u));
                }
                /* First letter of item name */
                put_tile(col, 17u, (u16)((u8)first - 32u));
            } else {
                put_tile(col, 17u, TILE_DOT);
            }
        }
    }

    /* --- Row 18: HP bar (left, red) ------------------------------------ */
    text_draw(0u, 18u, "HP");
    draw_bar(2u, 18u, stats->hp, stats->max_hp, RGB15(31, 0, 0));

    /* --- Row 18: Hunger bar (right, orange) at column 22 --------------- */
    text_draw(22u, 18u, "HG");
    /* Hunger bar: 6 tiles wide to fit in remaining columns (22+2 = 24, up to 30) */
    {
        s16 filled;
        u8  col;
        u8  hbar_x = 24u;
        u8  hbar_w = 6u;

        if (stats->max_hunger <= 0) {
            filled = 0;
        } else if (stats->hunger <= 0) {
            filled = 0;
        } else if (stats->hunger >= stats->max_hunger) {
            filled = (s16)hbar_w;
        } else {
            filled = (s16)((s32)stats->hunger * hbar_w / (s32)stats->max_hunger);
        }

        for (col = 0u; col < hbar_w; col++) {
            u16 tile = (col < (u8)filled) ? TILE_BLOCK : TILE_DOT;
            put_tile((u8)(hbar_x + col), 18u, tile);
        }
    }

    /* --- Row 19: Stamina bar (left, yellow) ----------------------------- */
    text_draw(0u, 19u, "ST");
    draw_bar(2u, 19u, stats->stamina, stats->max_stamina, RGB15(31, 31, 0));
}
