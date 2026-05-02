#include "farming.h"
#include "../world/section.h"
#include "../world/tile.h"

/* --------------------------------------------------------------------------
 * Module-private state
 * -------------------------------------------------------------------------- */

/* One CropTile entry per tile position in the active section. */
static CropTile  crop_tiles[SECTION_TILES_W * SECTION_TILES_H] __attribute__((section(".sbss")));

/* Pointer to the currently loaded section, set externally via
 * farming_set_section() or treated as NULL-safe throughout. */
static Section  *active_section = (void*)0;

/* --------------------------------------------------------------------------
 * Public: allow the section manager to register the active section.
 * (Not declared in the header but used internally; keeping it static-file
 *  linkage so the linker does not complain about a missing prototype.)
 * -------------------------------------------------------------------------- */
void farming_set_section(Section *s)
{
    active_section = s;
}

/* --------------------------------------------------------------------------
 * season_from_day
 *
 *   Full year = 96 days (4 seasons × 24 days).
 *   Season is determined by day % 96:
 *     [  0, 24) → SPRING
 *     [ 24, 48) → SUMMER
 *     [ 48, 72) → AUTUMN
 *     [ 72, 96) → WINTER
 * -------------------------------------------------------------------------- */
Season season_from_day(u16 day)
{
    u8 pos = (u8)(day % 96u);
    if (pos < 24u) return SEASON_SPRING;
    if (pos < 48u) return SEASON_SUMMER;
    if (pos < 72u) return SEASON_AUTUMN;
    return SEASON_WINTER;
}

/* --------------------------------------------------------------------------
 * farming_till
 *
 *   Zero out the CropTile slot and set the section tile to TILE_TILLED.
 * -------------------------------------------------------------------------- */
void farming_till(u8 x, u8 y)
{
    u16 idx = (u16)y * SECTION_TILES_W + (u16)x;
    CropTile *ct = &crop_tiles[idx];
    ct->seed_type    = SEED_BASIC;
    ct->water_day    = 0u;
    ct->growth_stage = 0u;

    if (active_section)
        section_set_tile(active_section, x, y, TILE_TILLED);
}

/* --------------------------------------------------------------------------
 * farming_plant
 *
 *   Only plants if the tile is currently TILE_TILLED.
 *   Sets seed_type; growth_stage stays 0 (planted but not yet growing).
 * -------------------------------------------------------------------------- */
void farming_plant(u8 x, u8 y, SeedType seed)
{
    if (!active_section) return;
    if (section_get_tile(active_section, x, y) != TILE_TILLED) return;

    u16 idx = (u16)y * SECTION_TILES_W + (u16)x;
    crop_tiles[idx].seed_type    = seed;
    crop_tiles[idx].water_day    = 0u;
    crop_tiles[idx].growth_stage = 0u;
}

/* --------------------------------------------------------------------------
 * farming_water
 *
 *   Record the day on which the tile was last watered.
 * -------------------------------------------------------------------------- */
void farming_water(u8 x, u8 y, u16 current_day)
{
    u16 idx = (u16)y * SECTION_TILES_W + (u16)x;
    crop_tiles[idx].water_day = current_day;
}

/* --------------------------------------------------------------------------
 * farming_tick
 *
 *   Called once per in-game day.
 *   A crop tile advances one growth stage if it was watered on
 *   current_day OR on (current_day - 1) — i.e. today or yesterday.
 *
 *   Tile visual mapping:
 *     stage 0 → TILE_TILLED   (planted, waiting)
 *     stage 1 → TILE_CROP_1
 *     stage 2 → TILE_CROP_2
 *     stage 3 → TILE_CROP_2   (still growing)
 *     stage 4 → TILE_CROP_3   (ready to harvest)
 * -------------------------------------------------------------------------- */
void farming_tick(u16 current_day)
{
    u16 i;
    u16 total = (u16)(SECTION_TILES_W * SECTION_TILES_H);

    for (i = 0u; i < total; ++i)
    {
        CropTile *ct = &crop_tiles[i];

        /* Skip tiles that have no planted crop or are already ready. */
        if (ct->growth_stage == 0u && ct->water_day == 0u) continue;
        if (ct->growth_stage >= 4u) continue;

        /* Check watered condition: today or yesterday.
         * Guard against underflow on day 0. */
        u16 prev_day = (current_day > 0u) ? (u16)(current_day - 1u) : 0u;
        bool watered = (ct->water_day == current_day) ||
                       (ct->water_day == prev_day && ct->water_day != 0u);

        if (watered)
        {
            ct->growth_stage = (u8)(ct->growth_stage + 1u);
            if (ct->growth_stage > 4u) ct->growth_stage = 4u;
        }

        /* Update the visual tile in the active section. */
        if (active_section)
        {
            u8 tx = (u8)(i % SECTION_TILES_W);
            u8 ty = (u8)(i / SECTION_TILES_W);
            TileType visual;
            switch (ct->growth_stage)
            {
                case 0u: visual = TILE_TILLED;  break;
                case 1u: visual = TILE_CROP_1;  break;
                case 2u: visual = TILE_CROP_2;  break;
                case 3u: visual = TILE_CROP_2;  break;
                default: visual = TILE_CROP_3;  break;
            }
            section_set_tile(active_section, tx, ty, visual);
        }
    }
}

/* --------------------------------------------------------------------------
 * farming_harvest
 *
 *   Harvests the crop at (x,y) if growth_stage == 4.
 *   Resets tile to TILE_DIRT and zeroes out the CropTile slot.
 *   Returns TRUE on success, FALSE if not ready.
 * -------------------------------------------------------------------------- */
bool farming_harvest(u8 x, u8 y)
{
    u16 idx = (u16)y * SECTION_TILES_W + (u16)x;
    CropTile *ct = &crop_tiles[idx];

    if (ct->growth_stage != 4u) return FALSE;

    /* Reset crop state. */
    ct->seed_type    = SEED_BASIC;
    ct->water_day    = 0u;
    ct->growth_stage = 0u;

    /* Restore tile to bare dirt. */
    if (active_section)
        section_set_tile(active_section, x, y, TILE_DIRT);

    return TRUE;
}
