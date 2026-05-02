/*
 * renderer.c — BG tile renderer for HollowShore.
 *
 * Hardware configuration (Mode 0):
 *   Charblock 0 (tile data)  : 0x06000000 – 0x06003FFF  (16 KB)
 *   Screenblock 8 (BG0 map)  : 0x06004000 – 0x06005FFF  (2 KB per 32×32 map)
 *
 * BG0CNT encoding used:
 *   bits  1-0  : charblock base (0 → 0x06000000)
 *   bit   6    : mosaic (0 = off)
 *   bit   7    : colour mode (0 = 4bpp)
 *   bits 12-8  : screenblock base (8 → 0x06004000)
 *   bits 15-14 : map size (00 = 256×256, one 32×32 screenblock)
 *
 * World tile ↔ hardware tile:
 *   Each game world tile is 16×16 pixels = 2×2 GBA hardware tiles (8×8 px each).
 *   set_bg_tile() writes a single hardware tile entry.
 *   section_render_visible() maps world tiles to 2×2 hardware tiles per entry.
 *
 * Screenblock coordinate system:
 *   BG0 map is 32×32 hardware tile entries (256×256 px screen space).
 *   With camera scroll via REG_BG0HOFS/VOFS the map wraps, giving a virtual
 *   512×512 surface (the hardware repeats the 32×32 block).
 *
 * Visible area at any one time: 240×160 screen pixels = 30×20 hardware tiles
 *   = 15×10 world tiles (since each world tile = 2×2 hardware tiles).
 */

#include "renderer.h"
#include "../gba.h"
#include "../systems/building.h"

/* ---- Constants ----------------------------------------------------------- */

/* Screenblock 8 base address */
#define BG0_SCREENBLOCK  ((vu16*)0x06004000)

/* Charblock 0 base (same as MEM_VRAM, but typed as u32* for tile writes) */
#define CHARBLOCK0       ((vu32*)0x06000000)

/* Screenblock dimensions (in hardware tiles) */
#define SB_WIDTH  32
#define SB_HEIGHT 32

/* Visible screen in hardware tiles */
#define SCREEN_HW_TILES_W  30   /* 240 px / 8 px */
#define SCREEN_HW_TILES_H  20   /* 160 px / 8 px */

/* Visible screen in world tiles (each world tile = 2×2 hw tiles) */
#define VISIBLE_WORLD_W  15     /* ceil(240/16) */
#define VISIBLE_WORLD_H  10     /* ceil(160/16) */

/* BG0CNT value:
 *   charblock 0 (bits 1-0 = 0),
 *   4bpp (bit 7 = 0),
 *   screenblock 8 (bits 12-8 = 0b01000),
 *   size 00 = 256×256 (bits 15-14 = 0).
 */
#define BG0CNT_VALUE  ((u16)((8u << 8) | (0u << 2) | 0u))

/* ---- Public API ---------------------------------------------------------- */

/*
 * init_renderer — configure BG0 for Mode 0 tile rendering and enable display.
 */
void init_renderer(void)
{
    /* BG0: charblock 0, screenblock 8, 4bpp, 256×256 map */
    REG_BG0CNT = BG0CNT_VALUE;

    /* Enable Mode 0 + BG0 + OBJ */
    REG_DISPCNT = MODE0 | BG0_ENABLE | OBJ_ENABLE;

    /* Reset scroll */
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;
}

/*
 * load_bg_tiles — copy tile_count 4bpp 8×8 tiles (32 bytes each = 8 u32 each)
 * into charblock 0 starting at tile index 0.
 *
 * tile_data must point to an array of tile_count * 8 u32 values.
 */
void load_bg_tiles(const u32 *tile_data, u16 tile_count)
{
    vu32 *dest = CHARBLOCK0;
    u32   word_count = (u32)tile_count * 8u; /* 8 u32 per 4bpp 8×8 tile */
    u32   i;
    for (i = 0; i < word_count; ++i)
        dest[i] = tile_data[i];
}

/*
 * set_bg_tile — write a single hardware tile entry into the BG0 screenblock.
 *
 * x, y are in hardware tile coordinates (0–31 within the 32×32 screenblock).
 * tile_id is the index into charblock 0 (0–255 for 4bpp tiles).
 *
 * The screenblock entry is a 16-bit value:
 *   bits  9-0  : tile index
 *   bit  10    : horizontal flip
 *   bit  11    : vertical flip
 *   bits 15-12 : palette bank (for 4bpp mode)
 *
 * We write tile_id only (no flip, palette 0); callers can OR in palette bits.
 */
void set_bg_tile(u8 x, u8 y, u8 tile_id)
{
    u16 idx = (u16)((u16)(y & 0x1Fu) * SB_WIDTH + (u16)(x & 0x1Fu));
    BG0_SCREENBLOCK[idx] = (u16)tile_id;
}

/*
 * section_render_visible — render the visible portion of section s.
 *
 * The world is SECTION_TILES_W × SECTION_TILES_H (160×160) world tiles.
 * The camera (cam_x, cam_y) is in pixel coordinates.
 *
 * Approach:
 *   1. Determine the top-left world tile from the camera pixel position.
 *   2. Iterate VISIBLE_WORLD_W × VISIBLE_WORLD_H world tiles.
 *   3. For each world tile, write 4 hardware tile entries (2×2) into the
 *      BG0 screenblock at the corresponding wrapped hardware tile position.
 *   4. Set REG_BG0HOFS/VOFS so the hardware scroll matches cam_x/cam_y.
 *
 * Hardware tile slot wrapping:
 *   The BG0 screenblock is 32×32 hw tiles.  We address it modulo 32, which
 *   naturally produces a toroidal (wrapping) tile map.  The hardware scroll
 *   registers then shift the view so it appears continuous.
 *
 * World tile to hardware tile mapping:
 *   Each world tile wx,wy maps to hw tile pair (wx*2, wy*2) and (wx*2+1, wy*2+1).
 *   Four entries: (hw_x, hw_y), (hw_x+1, hw_y), (hw_x, hw_y+1), (hw_x+1, hw_y+1).
 *   The world tile's tile_id (a TileType enum value) is used as the GBA hw tile
 *   index directly — callers must load matching tile graphics at those indices.
 *
 * We render VISIBLE_WORLD_W+1 × VISIBLE_WORLD_H+1 world tiles to cover partial
 * tiles at the screen edges (due to sub-tile pixel scroll).
 */
void section_render_visible(const Section *s, s16 cam_x, s16 cam_y)
{
    /* World tile at top-left corner of the camera view */
    s16 start_wx = cam_x / (s16)TILE_SIZE;
    s16 start_wy = cam_y / (s16)TILE_SIZE;

    /* Render one extra tile in each direction to cover partial tiles */
    s16 wx, wy;
    for (wy = start_wy; wy < start_wy + (s16)(VISIBLE_WORLD_H + 1); ++wy) {
        for (wx = start_wx; wx < start_wx + (s16)(VISIBLE_WORLD_W + 1); ++wx) {
            /* Clamp to section bounds */
            u8 tile_id;
            u8 sec_wx, sec_wy;

            if (wx < 0 || wx >= (s16)SECTION_TILES_W ||
                wy < 0 || wy >= (s16)SECTION_TILES_H) {
                tile_id = 0; /* default tile (grass or blank) for out-of-bounds */
            } else {
                sec_wx = (u8)wx;
                sec_wy = (u8)wy;
                tile_id = s->tiles[(u16)sec_wy * SECTION_TILES_W + sec_wx];
            }

            /* Hardware tile position (wrap into 32×32 screenblock) */
            u8 hw_x = (u8)((wx * 2) & 0x1F);
            u8 hw_y = (u8)((wy * 2) & 0x1F);
            u8 hw_x1 = (u8)((hw_x + 1) & 0x1F);
            u8 hw_y1 = (u8)((hw_y + 1) & 0x1F);

            /* Each world tile uses 4 adjacent hw tile entries.
             * We use the same tile_id for all 4; if multi-hw-tile art is
             * desired in a future wave, the tile_data array can be structured
             * so that tile_id*4+offset gives the correct sub-tile. */
            set_bg_tile(hw_x,  hw_y,  tile_id);
            set_bg_tile(hw_x1, hw_y,  tile_id);
            set_bg_tile(hw_x,  hw_y1, tile_id);
            set_bg_tile(hw_x1, hw_y1, tile_id);
        }
    }

    /* Apply hardware scroll so the visible region aligns with cam_x/cam_y */
    REG_BG0HOFS = (u16)((u16)cam_x & 0x1FFu);
    REG_BG0VOFS = (u16)((u16)cam_y & 0x1FFu);
}

/*
 * render_structures — write BG tile entries for all placed structures
 * in the current section.
 *
 * placed_structures[] and structure_count are extern from building.h.
 * Each structure occupies one world tile position (x, y).  We map it to
 * the 2×2 hardware tile block and write tile entries derived from the
 * structure type.
 *
 * Tile index convention: structure tile indices start at TILE_COUNT (12)
 * so they don't overlap the terrain tiles loaded at indices 0–11.
 * The base tile for a structure type is: TILE_COUNT + (u8)type.
 */
void render_structures(const Section *s)
{
    u8 i;
    (void)s; /* section pointer reserved for future layer/section checks */

    for (i = 0; i < structure_count; ++i) {
        const Structure *st = &placed_structures[i];
        if (st->type == STRUCT_NONE) continue;

        u8 hw_x  = (u8)((st->x * 2) & 0x1Fu);
        u8 hw_y  = (u8)((st->y * 2) & 0x1Fu);
        u8 hw_x1 = (u8)((hw_x + 1u) & 0x1Fu);
        u8 hw_y1 = (u8)((hw_y + 1u) & 0x1Fu);

        /* Structure tile base: offset past terrain tile range */
        u8 tile_id = (u8)(12u + (u8)st->type);

        set_bg_tile(hw_x,  hw_y,  tile_id);
        set_bg_tile(hw_x1, hw_y,  tile_id);
        set_bg_tile(hw_x,  hw_y1, tile_id);
        set_bg_tile(hw_x1, hw_y1, tile_id);
    }
}
