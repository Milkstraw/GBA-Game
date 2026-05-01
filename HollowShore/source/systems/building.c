#include "building.h"

/* Task 5.3 — implement building_place, building_break, building_upgrade,
              lighting_radius, tile_in_light */

Structure placed_structures[MAX_STRUCTURES_PER_SECTION];
u8        structure_count = 0;
