#include "building.h"

/* --------------------------------------------------------------------------
 * Global structure storage (declared extern in building.h)
 * -------------------------------------------------------------------------- */
Structure placed_structures[MAX_STRUCTURES_PER_SECTION];
u8        structure_count = 0u;

/* --------------------------------------------------------------------------
 * default_hp
 *
 *   Returns the starting HP for a newly placed structure.
 * -------------------------------------------------------------------------- */
static u8 default_hp(StructureType type)
{
    switch (type)
    {
        case STRUCT_WALL_WOOD:      return 10u;
        case STRUCT_WALL_STONE:     return 14u;
        case STRUCT_WALL_BRICK:     return 18u;
        case STRUCT_WALL_METAL:     return 24u;
        case STRUCT_TORCH:          return  3u;
        case STRUCT_LANTERN:        return  4u;
        case STRUCT_BRAZIER:        return  5u;
        case STRUCT_CAMPFIRE:       return  5u;
        case STRUCT_BED:            return  6u;
        case STRUCT_WORKBENCH:      return  8u;
        case STRUCT_FORGE:          return 12u;
        case STRUCT_ALCHEMY_TABLE:  return  8u;
        case STRUCT_FENCE:          return  6u;
        case STRUCT_DOOR:           return  8u;
        default:                    return  5u;
    }
}

/* --------------------------------------------------------------------------
 * building_place
 *
 *   Appends a new structure to placed_structures[] if capacity allows.
 *   A structure at the same (x,y) is not checked for duplicates here —
 *   the caller is responsible for grid validity.
 * -------------------------------------------------------------------------- */
void building_place(u8 x, u8 y, StructureType type)
{
    if (type == STRUCT_NONE || type >= STRUCT_COUNT) return;
    if (structure_count >= MAX_STRUCTURES_PER_SECTION) return;

    Structure *s = &placed_structures[structure_count];
    s->type = type;
    s->tier = 0u;
    s->x    = x;
    s->y    = y;
    s->hp   = default_hp(type);

    ++structure_count;
}

/* --------------------------------------------------------------------------
 * building_break
 *
 *   Finds the structure at (x,y).  If found, replaces it with the last
 *   entry in the array (swap-and-pop) and decrements structure_count.
 * -------------------------------------------------------------------------- */
void building_break(u8 x, u8 y)
{
    u8 i;

    for (i = 0u; i < structure_count; ++i)
    {
        if (placed_structures[i].x == x && placed_structures[i].y == y)
        {
            /* Swap with the last element (handles the case i == last). */
            placed_structures[i] = placed_structures[structure_count - 1u];
            --structure_count;
            return;
        }
    }
}

/* --------------------------------------------------------------------------
 * building_upgrade
 *
 *   Finds the structure at (x,y) and increments its tier (max 3) and HP.
 *   Tier progression corresponds to Wood→Stone→Brick→Metal for walls,
 *   or a generic quality upgrade for other structures.
 * -------------------------------------------------------------------------- */
void building_upgrade(u8 x, u8 y)
{
    u8 i;

    for (i = 0u; i < structure_count; ++i)
    {
        if (placed_structures[i].x == x && placed_structures[i].y == y)
        {
            Structure *s = &placed_structures[i];
            if (s->tier < 3u)
            {
                s->tier = (u8)(s->tier + 1u);
                /* Add 4 HP per tier upgrade, capped at 255. */
                u16 new_hp = (u16)s->hp + 4u;
                s->hp = (new_hp > 255u) ? 255u : (u8)new_hp;
            }
            return;
        }
    }
}

/* --------------------------------------------------------------------------
 * lighting_radius
 *
 *   Returns the Manhattan-distance light radius of a structure type.
 *   Returns 0 for non-light-emitting structures.
 * -------------------------------------------------------------------------- */
u8 lighting_radius(StructureType type)
{
    switch (type)
    {
        case STRUCT_TORCH:    return 3u;
        case STRUCT_LANTERN:  return 5u;
        case STRUCT_BRAZIER:  return 4u;
        case STRUCT_CAMPFIRE: return 4u;
        default:              return 0u;
    }
}

/* --------------------------------------------------------------------------
 * tile_in_light
 *
 *   Returns TRUE if tile (x,y) is within the lighting radius of at least
 *   one placed light source (Manhattan distance).
 * -------------------------------------------------------------------------- */
bool tile_in_light(u8 x, u8 y)
{
    u8 i;

    for (i = 0u; i < structure_count; ++i)
    {
        const Structure *s = &placed_structures[i];
        u8 radius = lighting_radius(s->type);

        if (radius == 0u) continue;

        /* Manhattan distance, avoiding signed underflow. */
        u8 dx = (s->x >= x) ? (u8)(s->x - x) : (u8)(x - s->x);
        u8 dy = (s->y >= y) ? (u8)(s->y - y) : (u8)(y - s->y);
        u8 dist = (u8)(dx + dy);

        if (dist <= radius) return TRUE;
    }

    return FALSE;
}
