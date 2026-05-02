/*
 * section.c — World section management for HollowShore.
 *
 * Sections are arranged in a 3x3 grid, IDs 1–9 row-major:
 *   1 2 3
 *   4 5 6
 *   7 8 9
 *
 * SRAM layout for dirty-tile log:
 *   Each section gets 512 bytes at offset (id-1)*512.
 *   Byte 0    = count of dirty entries (max 170 entries fit in 511 bytes).
 *   Bytes 1+  = packed entries: u16 tile_index (2 bytes), u8 new_type (1 byte).
 *               Each entry = 3 bytes. 510 / 3 = 170 entries max.
 */

#include "section.h"
#include "maps/world_a.h"
#include "maps/world_b.h"
#include "maps/world_c.h"

/* SRAM base address — 8-bit bus, access byte-by-byte */
#define SRAM_BASE      ((vu8 *)0x0E000000)
#define SRAM_SECTION_STRIDE   512u   /* bytes reserved per section */
#define DIRTY_LOG_MAX         170u   /* max dirty-tile entries per section */

/* -------------------------------------------------------------------------- */
/* Internal helpers                                                            */
/* -------------------------------------------------------------------------- */

/* Map a section id (1–9) to the matching world_a ROM tile array. */
static const u8 *section_rom_data(u8 id)
{
    switch (id) {
    case 1: return world_a_section1;
    case 2: return world_a_section2;
    case 3: return world_a_section3;
    case 4: return world_a_section4;
    case 5: return world_a_section5;
    case 6: return world_a_section6;
    case 7: return world_a_section7;
    case 8: return world_a_section8;
    case 9: return world_a_section9;
    default: return world_a_section1;
    }
}

/* Copy 25600 bytes from a ROM array into the Section tile buffer, then apply
   any dirty-tile overrides stored in SRAM for this section. */
static void section_copy_tiles(Section *out, const u8 *rom)
{
    u32 total = (u32)SECTION_TILES_W * (u32)SECTION_TILES_H;
    u32 i;
    for (i = 0; i < total; ++i) {
        out->tiles[i] = rom[i];
    }
}

/* Apply dirty-tile log from SRAM on top of the freshly copied tile array. */
static void section_apply_dirty_log(Section *out)
{
    u32 base = (u32)(out->id - 1u) * SRAM_SECTION_STRIDE;
    u8 count = SRAM_BASE[base];
    u32 entry;

    if (count > DIRTY_LOG_MAX) {
        return; /* corrupt log — ignore */
    }

    for (entry = 0; entry < (u32)count; ++entry) {
        u32 off = base + 1u + entry * 3u;
        u16 idx = (u16)((u16)SRAM_BASE[off] | ((u16)SRAM_BASE[off + 1u] << 8u));
        u8  type = SRAM_BASE[off + 2u];

        if (idx < (u16)(SECTION_TILES_W * SECTION_TILES_H) &&
            type < (u8)TILE_COUNT) {
            out->tiles[idx] = type;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

void section_load(u8 id, Section *out)
{
    const u8 *rom;
    u32 i;
    u32 total = (u32)SECTION_TILES_W * (u32)SECTION_TILES_H;

    if (id < 1u || id > 9u) {
        id = 1u;
    }

    out->id    = id;
    out->cam_x = 0;
    out->cam_y = 0;

    /* Copy ROM tile data */
    rom = section_rom_data(id);
    section_copy_tiles(out, rom);

    /* Clear resource nodes */
    for (i = 0; i < total; ++i) {
        out->resources[i].tile_id     = 0;
        out->resources[i].respawn_day = 0;
    }

    /* Re-apply any player-made tile changes stored in SRAM */
    section_apply_dirty_log(out);
}

void section_save_state(const Section *s)
{
    /*
     * Compare current tiles against the matching ROM array and write
     * only the differences (dirty tiles) into the SRAM log.
     *
     * Format at SRAM offset (id-1)*512:
     *   [0]        = count (u8)
     *   [1 + n*3]  = index_lo (u8)
     *   [2 + n*3]  = index_hi (u8)
     *   [3 + n*3]  = tile type (u8)
     */
    u32 base = (u32)(s->id - 1u) * SRAM_SECTION_STRIDE;
    const u8 *rom = section_rom_data(s->id);
    u32 total = (u32)SECTION_TILES_W * (u32)SECTION_TILES_H;
    u8  count = 0;
    u32 i;

    for (i = 0; i < total && (u32)count < DIRTY_LOG_MAX; ++i) {
        if (s->tiles[i] != rom[i]) {
            u32 off = base + 1u + (u32)count * 3u;
            SRAM_BASE[off]      = (u8)(i & 0xFFu);
            SRAM_BASE[off + 1u] = (u8)((i >> 8u) & 0xFFu);
            SRAM_BASE[off + 2u] = s->tiles[i];
            ++count;
        }
    }

    /* Write count last so a partial write doesn't corrupt the log */
    SRAM_BASE[base] = count;
}

void section_set_tile(Section *s, u8 x, u8 y, TileType type)
{
    if (x >= (u8)SECTION_TILES_W || y >= (u8)SECTION_TILES_H) {
        return;
    }
    s->tiles[(u32)y * (u32)SECTION_TILES_W + (u32)x] = (u8)type;
}

TileType section_get_tile(const Section *s, u8 x, u8 y)
{
    if (x >= (u8)SECTION_TILES_W || y >= (u8)SECTION_TILES_H) {
        return TILE_GRASS;
    }
    return (TileType)s->tiles[(u32)y * (u32)SECTION_TILES_W + (u32)x];
}

u8 section_id_from_direction(u8 current_id, u8 dir)
{
    u8 id = current_id;

    if (id < 1u || id > 9u) {
        return current_id;
    }

    switch ((Direction)dir) {
    case DIR_RIGHT:
        /* Wrap only if not in rightmost column (id % 3 != 0) */
        if ((id % 3u) != 0u) {
            id = id + 1u;
        }
        break;
    case DIR_LEFT:
        /* Wrap only if not in leftmost column (id % 3 != 1) */
        if ((id % 3u) != 1u) {
            id = id - 1u;
        }
        break;
    case DIR_DOWN:
        if (id <= 6u) {
            id = id + 3u;
        }
        break;
    case DIR_UP:
        if (id >= 4u) {
            id = id - 3u;
        }
        break;
    default:
        break;
    }

    return id;
}
