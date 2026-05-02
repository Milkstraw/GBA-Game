/*
 * world_c.c — World C: The Hollow Depths (ROM tile data, 9 × 25600 bytes).
 *
 * Tile IDs (from tile.h):
 *   0=GRASS   1=DIRT   2=WATER   3=TREE    4=ROCK
 *   5=TILLED  6=CROP_1 7=CROP_2  8=CROP_3
 *   9=LAVA   10=ICE   11=CORRUPTION
 *
 * Theme:
 *   - Sections 1–3 (north row): heavy ICE presence, ROCK outcrops, sparse GRASS
 *   - Sections 4, 6 (middle flanks): CORRUPTION spreading from edges, LAVA rivers
 *   - Section 5 (centre): Hollow Throne entrance — ROCK ring, CORRUPTION surround
 *   - Sections 7–9 (south row): LAVA rivers, CORRUPTION patches, ash DIRT paths
 *
 * All sections: 160 rows × 160 cols = 25600 bytes.
 * Row 0 and row 159 = TREE_ROW border.
 * Interior rows 1–158 (158 rows).
 * 158 = 39 × 4 + 2  ⟹  39 full 4-row cycles + 2 extra rows.
 */

#include "world_c.h"

/* Tile shorthands */
#define G   0   /* GRASS      */
#define D   1   /* DIRT       */
#define W   2   /* WATER      */
#define T   3   /* TREE       */
#define R   4   /* ROCK       */
#define L   9   /* LAVA       */
#define I  10   /* ICE        */
#define C  11   /* CORRUPTION */

/* =========================================================================
 * Row templates — 160 values each
 * ========================================================================= */

/* Solid tree border */
#define TREE_ROW \
    T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,\
    T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,\
    T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,\
    T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,\
    T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,\
    T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,\
    T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,\
    T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T

/* --- Northern sections (ICE-heavy) --------------------------------------- */

/* Solid ice field */
#define ICE_ROW \
    T,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,T

/* Ice with rock outcrops */
#define ICE_ROCK_ROW \
    T,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    R,R,R,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,R,R,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,T

/* Ice with sparse corruption patches */
#define ICE_CORRUPT_ROW \
    T,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,C,C,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,C,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,C,C,C,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,T

/* Ice with grass patches (transitional) */
#define ICE_GRASS_ROW \
    T,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    G,G,G,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,G,G,G,G,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,\
    I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,I,T

/* --- Middle sections (CORRUPTION + LAVA) --------------------------------- */

/* Corruption field with dirt paths */
#define CORRUPT_ROW \
    T,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,T

/* Corruption with rock clusters */
#define CORRUPT_ROCK_ROW \
    T,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    R,R,R,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    R,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,R,R,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,T

/* Lava river through corruption */
#define LAVA_RIVER_ROW \
    T,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,T

/* Corruption with dirt veins */
#define CORRUPT_DIRT_ROW \
    T,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,T

/* --- Centre section (Hollow Throne) -------------------------------------- */

/* Open corruption with central rock ring for Hollow Throne entrance */
#define THRONE_ROW \
    T,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,\
    C,C,C,C,C,C,C,R,R,R,R,R,R,R,R,R,R,C,C,C,\
    C,C,C,C,C,C,C,R,C,C,C,C,C,C,C,C,R,C,C,C,\
    C,C,C,C,C,C,C,R,C,C,C,C,C,C,C,C,R,C,C,C,\
    C,C,C,C,C,C,C,R,R,R,R,R,R,R,R,R,R,C,C,C,\
    C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,C,T

/* --- Southern sections (LAVA-heavy) -------------------------------------- */

/* Solid lava field */
#define LAVA_ROW \
    T,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,T

/* Lava with rock islands */
#define LAVA_ROCK_ROW \
    T,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    R,R,R,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    R,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,R,R,R,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,T

/* Lava with corruption patches */
#define LAVA_CORRUPT_ROW \
    T,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    C,C,C,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    C,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,C,C,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,T

/* Lava with dirt ash paths */
#define LAVA_DIRT_ROW \
    T,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,D,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,\
    L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,L,T

/* =========================================================================
 * Section 1 — NW: heavy ice, rock outcrops, sparse corruption
 * Pattern: 39×4 + 2 = 158 interior rows
 * ========================================================================= */
const u8 world_c_section1[160 * 160] = {
    TREE_ROW,                                                            /* row 0 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*  1-4  */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*  5-8  */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*  9-12 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 13-16 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 17-20 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 21-24 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 25-28 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 29-32 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 33-36 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 37-40 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 41-44 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 45-48 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 49-52 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 53-56 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 57-60 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 61-64 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 65-68 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 69-72 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 73-76 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 77-80 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 81-84 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 85-88 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 89-92 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 93-96 */
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /* 97-100*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*101-104*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*105-108*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*109-112*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*113-116*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*117-120*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*121-124*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*125-128*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*129-132*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*133-136*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*137-140*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*141-144*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*145-148*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*149-152*/
    ICE_ROW,       ICE_ROCK_ROW,  ICE_CORRUPT_ROW, ICE_GRASS_ROW,      /*153-156*/
    ICE_ROW,       ICE_ROCK_ROW,                                        /*157-158*/
    TREE_ROW                                                             /* row 159 */
};

/* =========================================================================
 * Section 2 — N-centre: solid ice with rock and corruption patches
 * ========================================================================= */
const u8 world_c_section2[160 * 160] = {
    TREE_ROW,
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*  1-4  */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*  5-8  */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*  9-12 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 13-16 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 17-20 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 21-24 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 25-28 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 29-32 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 33-36 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 37-40 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 41-44 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 45-48 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 49-52 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 53-56 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 57-60 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 61-64 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 65-68 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 69-72 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 73-76 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 77-80 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 81-84 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 85-88 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 89-92 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 93-96 */
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /* 97-100*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*101-104*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*105-108*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*109-112*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*113-116*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*117-120*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*121-124*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*125-128*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*129-132*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*133-136*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*137-140*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*141-144*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*145-148*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*149-152*/
    ICE_ROW,       ICE_ROW,       ICE_ROCK_ROW,   ICE_CORRUPT_ROW,    /*153-156*/
    ICE_ROW,       ICE_ROW,                                            /*157-158*/
    TREE_ROW
};

/* =========================================================================
 * Section 3 — NE: ice with grass remnants and corruption spreading
 * ========================================================================= */
const u8 world_c_section3[160 * 160] = {
    TREE_ROW,
    ICE_GRASS_ROW, ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,      /*  1-4  */
    ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,  ICE_GRASS_ROW,     /*  5-8  */
    ICE_ROW,       ICE_ROCK_ROW,    ICE_GRASS_ROW, ICE_CORRUPT_ROW,   /*  9-12 */
    ICE_ROCK_ROW,  ICE_GRASS_ROW,   ICE_CORRUPT_ROW, ICE_ROW,         /* 13-16 */
    ICE_GRASS_ROW, ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,      /* 17-20 */
    ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,  ICE_GRASS_ROW,     /* 21-24 */
    ICE_ROW,       ICE_ROCK_ROW,    ICE_GRASS_ROW, ICE_CORRUPT_ROW,   /* 25-28 */
    ICE_ROCK_ROW,  ICE_GRASS_ROW,   ICE_CORRUPT_ROW, ICE_ROW,         /* 29-32 */
    ICE_GRASS_ROW, ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,      /* 33-36 */
    ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,  ICE_GRASS_ROW,     /* 37-40 */
    ICE_ROW,       ICE_ROCK_ROW,    ICE_GRASS_ROW, ICE_CORRUPT_ROW,   /* 41-44 */
    ICE_ROCK_ROW,  ICE_GRASS_ROW,   ICE_CORRUPT_ROW, ICE_ROW,         /* 45-48 */
    ICE_GRASS_ROW, ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,      /* 49-52 */
    ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,  ICE_GRASS_ROW,     /* 53-56 */
    ICE_ROW,       ICE_ROCK_ROW,    ICE_GRASS_ROW, ICE_CORRUPT_ROW,   /* 57-60 */
    ICE_ROCK_ROW,  ICE_GRASS_ROW,   ICE_CORRUPT_ROW, ICE_ROW,         /* 61-64 */
    ICE_GRASS_ROW, ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,      /* 65-68 */
    ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,  ICE_GRASS_ROW,     /* 69-72 */
    ICE_ROW,       ICE_ROCK_ROW,    ICE_GRASS_ROW, ICE_CORRUPT_ROW,   /* 73-76 */
    ICE_ROCK_ROW,  ICE_GRASS_ROW,   ICE_CORRUPT_ROW, ICE_ROW,         /* 77-80 */
    ICE_GRASS_ROW, ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,      /* 81-84 */
    ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,  ICE_GRASS_ROW,     /* 85-88 */
    ICE_ROW,       ICE_ROCK_ROW,    ICE_GRASS_ROW, ICE_CORRUPT_ROW,   /* 89-92 */
    ICE_ROCK_ROW,  ICE_GRASS_ROW,   ICE_CORRUPT_ROW, ICE_ROW,         /* 93-96 */
    ICE_GRASS_ROW, ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,      /* 97-100*/
    ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,  ICE_GRASS_ROW,     /*101-104*/
    ICE_ROW,       ICE_ROCK_ROW,    ICE_GRASS_ROW, ICE_CORRUPT_ROW,   /*105-108*/
    ICE_ROCK_ROW,  ICE_GRASS_ROW,   ICE_CORRUPT_ROW, ICE_ROW,         /*109-112*/
    ICE_GRASS_ROW, ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,      /*113-116*/
    ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,  ICE_GRASS_ROW,     /*117-120*/
    ICE_ROW,       ICE_ROCK_ROW,    ICE_GRASS_ROW, ICE_CORRUPT_ROW,   /*121-124*/
    ICE_ROCK_ROW,  ICE_GRASS_ROW,   ICE_CORRUPT_ROW, ICE_ROW,         /*125-128*/
    ICE_GRASS_ROW, ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,      /*129-132*/
    ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,  ICE_GRASS_ROW,     /*133-136*/
    ICE_ROW,       ICE_ROCK_ROW,    ICE_GRASS_ROW, ICE_CORRUPT_ROW,   /*137-140*/
    ICE_ROCK_ROW,  ICE_GRASS_ROW,   ICE_CORRUPT_ROW, ICE_ROW,         /*141-144*/
    ICE_GRASS_ROW, ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,      /*145-148*/
    ICE_CORRUPT_ROW, ICE_ROW,       ICE_ROCK_ROW,  ICE_GRASS_ROW,     /*149-152*/
    ICE_ROW,       ICE_ROCK_ROW,    ICE_GRASS_ROW, ICE_CORRUPT_ROW,   /*153-156*/
    ICE_ROCK_ROW,  ICE_GRASS_ROW,                                      /*157-158*/
    TREE_ROW
};

/* =========================================================================
 * Section 4 — W: corruption spreading from edges, lava river running through
 * ========================================================================= */
const u8 world_c_section4[160 * 160] = {
    TREE_ROW,
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, /*  1-4  */
    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, CORRUPT_ROW,    /*  5-8  */
    LAVA_RIVER_ROW,   CORRUPT_DIRT_ROW, CORRUPT_ROW,  CORRUPT_ROCK_ROW, /*  9-12 */
    CORRUPT_DIRT_ROW, CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, /* 13-16 */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, /* 17-20 */
    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, CORRUPT_ROW,    /* 21-24 */
    LAVA_RIVER_ROW,   CORRUPT_DIRT_ROW, CORRUPT_ROW,  CORRUPT_ROCK_ROW, /* 25-28 */
    CORRUPT_DIRT_ROW, CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, /* 29-32 */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, /* 33-36 */
    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, CORRUPT_ROW,    /* 37-40 */
    LAVA_RIVER_ROW,   CORRUPT_DIRT_ROW, CORRUPT_ROW,  CORRUPT_ROCK_ROW, /* 41-44 */
    CORRUPT_DIRT_ROW, CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, /* 45-48 */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, /* 49-52 */
    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, CORRUPT_ROW,    /* 53-56 */
    LAVA_RIVER_ROW,   CORRUPT_DIRT_ROW, CORRUPT_ROW,  CORRUPT_ROCK_ROW, /* 57-60 */
    CORRUPT_DIRT_ROW, CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, /* 61-64 */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, /* 65-68 */
    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, CORRUPT_ROW,    /* 69-72 */
    LAVA_RIVER_ROW,   CORRUPT_DIRT_ROW, CORRUPT_ROW,  CORRUPT_ROCK_ROW, /* 73-76 */
    CORRUPT_DIRT_ROW, CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, /* 77-80 */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, /* 81-84 */
    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, CORRUPT_ROW,    /* 85-88 */
    LAVA_RIVER_ROW,   CORRUPT_DIRT_ROW, CORRUPT_ROW,  CORRUPT_ROCK_ROW, /* 89-92 */
    CORRUPT_DIRT_ROW, CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, /* 93-96 */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, /* 97-100*/
    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, CORRUPT_ROW,    /*101-104*/
    LAVA_RIVER_ROW,   CORRUPT_DIRT_ROW, CORRUPT_ROW,  CORRUPT_ROCK_ROW, /*105-108*/
    CORRUPT_DIRT_ROW, CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, /*109-112*/
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, /*113-116*/
    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, CORRUPT_ROW,    /*117-120*/
    LAVA_RIVER_ROW,   CORRUPT_DIRT_ROW, CORRUPT_ROW,  CORRUPT_ROCK_ROW, /*121-124*/
    CORRUPT_DIRT_ROW, CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, /*125-128*/
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, /*129-132*/
    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, CORRUPT_ROW,    /*133-136*/
    LAVA_RIVER_ROW,   CORRUPT_DIRT_ROW, CORRUPT_ROW,  CORRUPT_ROCK_ROW, /*137-140*/
    CORRUPT_DIRT_ROW, CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, /*141-144*/
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, /*145-148*/
    CORRUPT_ROCK_ROW, LAVA_RIVER_ROW, CORRUPT_DIRT_ROW, CORRUPT_ROW,    /*149-152*/
    LAVA_RIVER_ROW,   CORRUPT_DIRT_ROW, CORRUPT_ROW,  CORRUPT_ROCK_ROW, /*153-156*/
    CORRUPT_DIRT_ROW, CORRUPT_ROW,                                        /*157-158*/
    TREE_ROW
};

/* =========================================================================
 * Section 5 — Centre: Hollow Throne entrance.
 * Layout:
 *   rows   1-60:  approach corruption (60 rows = 15×4)
 *   rows  61-98:  throne ring rows   (38 rows)
 *   rows  99-158: corruption south   (60 rows = 15×4)
 *   Total interior = 60 + 38 + 60 = 158. Correct.
 * ========================================================================= */
const u8 world_c_section5[160 * 160] = {
    TREE_ROW,                                                            /* row 0 */
    /* rows 1-60: approach (60 rows) */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*  1-4  */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*  5-8  */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*  9-12 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 13-16 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 17-20 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 21-24 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 25-28 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 29-32 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 33-36 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 37-40 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 41-44 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 45-48 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 49-52 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 53-56 */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 57-60 */
    /* rows 61-98: hollow throne ring (38 rows) */
    THRONE_ROW, THRONE_ROW, THRONE_ROW, THRONE_ROW,                    /* 61-64 */
    THRONE_ROW, THRONE_ROW, THRONE_ROW, THRONE_ROW,                    /* 65-68 */
    THRONE_ROW, THRONE_ROW, THRONE_ROW, THRONE_ROW,                    /* 69-72 */
    THRONE_ROW, THRONE_ROW, THRONE_ROW, THRONE_ROW,                    /* 73-76 */
    THRONE_ROW, THRONE_ROW, THRONE_ROW, THRONE_ROW,                    /* 77-80 */
    THRONE_ROW, THRONE_ROW, THRONE_ROW, THRONE_ROW,                    /* 81-84 */
    THRONE_ROW, THRONE_ROW, THRONE_ROW, THRONE_ROW,                    /* 85-88 */
    THRONE_ROW, THRONE_ROW, THRONE_ROW, THRONE_ROW,                    /* 89-92 */
    THRONE_ROW, THRONE_ROW, THRONE_ROW, THRONE_ROW,                    /* 93-96 */
    THRONE_ROW, THRONE_ROW,                                             /* 97-98 */
    /* rows 99-158: corruption south (60 rows) */
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /* 99-102*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*103-106*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*107-110*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*111-114*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*115-118*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*119-122*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*123-126*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*127-130*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*131-134*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*135-138*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*139-142*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*143-146*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*147-150*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*151-154*/
    CORRUPT_ROW, CORRUPT_ROW, CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW,      /*155-158*/
    TREE_ROW                                                             /* row 159 */
};

/* =========================================================================
 * Section 6 — E: corruption with lava rivers and rock outcrops
 * ========================================================================= */
const u8 world_c_section6[160 * 160] = {
    TREE_ROW,
    LAVA_RIVER_ROW,   CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, /*  1-4  */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,   /*  5-8  */
    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW, CORRUPT_ROW,      /*  9-12 */
    CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,  CORRUPT_ROW,   CORRUPT_ROCK_ROW,   /* 13-16 */
    LAVA_RIVER_ROW,   CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, /* 17-20 */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,   /* 21-24 */
    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW, CORRUPT_ROW,      /* 25-28 */
    CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,  CORRUPT_ROW,   CORRUPT_ROCK_ROW,   /* 29-32 */
    LAVA_RIVER_ROW,   CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, /* 33-36 */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,   /* 37-40 */
    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW, CORRUPT_ROW,      /* 41-44 */
    CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,  CORRUPT_ROW,   CORRUPT_ROCK_ROW,   /* 45-48 */
    LAVA_RIVER_ROW,   CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, /* 49-52 */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,   /* 53-56 */
    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW, CORRUPT_ROW,      /* 57-60 */
    CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,  CORRUPT_ROW,   CORRUPT_ROCK_ROW,   /* 61-64 */
    LAVA_RIVER_ROW,   CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, /* 65-68 */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,   /* 69-72 */
    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW, CORRUPT_ROW,      /* 73-76 */
    CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,  CORRUPT_ROW,   CORRUPT_ROCK_ROW,   /* 77-80 */
    LAVA_RIVER_ROW,   CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, /* 81-84 */
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,   /* 85-88 */
    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW, CORRUPT_ROW,      /* 89-92 */
    CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,  CORRUPT_ROW,   CORRUPT_ROCK_ROW,   /* 93-96 */
    LAVA_RIVER_ROW,   CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, /* 97-100*/
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,   /*101-104*/
    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW, CORRUPT_ROW,      /*105-108*/
    CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,  CORRUPT_ROW,   CORRUPT_ROCK_ROW,   /*109-112*/
    LAVA_RIVER_ROW,   CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, /*113-116*/
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,   /*117-120*/
    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW, CORRUPT_ROW,      /*121-124*/
    CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,  CORRUPT_ROW,   CORRUPT_ROCK_ROW,   /*125-128*/
    LAVA_RIVER_ROW,   CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, /*129-132*/
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,   /*133-136*/
    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW, CORRUPT_ROW,      /*137-140*/
    CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,  CORRUPT_ROW,   CORRUPT_ROCK_ROW,   /*141-144*/
    LAVA_RIVER_ROW,   CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, /*145-148*/
    CORRUPT_ROW,    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,   /*149-152*/
    CORRUPT_ROCK_ROW, CORRUPT_DIRT_ROW, LAVA_RIVER_ROW, CORRUPT_ROW,      /*153-156*/
    CORRUPT_DIRT_ROW, LAVA_RIVER_ROW,                                       /*157-158*/
    TREE_ROW
};

/* =========================================================================
 * Section 7 — SW: heavy lava with corruption and rock islands
 * ========================================================================= */
const u8 world_c_section7[160 * 160] = {
    TREE_ROW,
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,   /*  1-4  */
    LAVA_ROCK_ROW, LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,  LAVA_ROW,        /*  5-8  */
    LAVA_CORRUPT_ROW, LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /*  9-12 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW,  /* 13-16 */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,   /* 17-20 */
    LAVA_ROCK_ROW, LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,  LAVA_ROW,        /* 21-24 */
    LAVA_CORRUPT_ROW, LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /* 25-28 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW,  /* 29-32 */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,   /* 33-36 */
    LAVA_ROCK_ROW, LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,  LAVA_ROW,        /* 37-40 */
    LAVA_CORRUPT_ROW, LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /* 41-44 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW,  /* 45-48 */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,   /* 49-52 */
    LAVA_ROCK_ROW, LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,  LAVA_ROW,        /* 53-56 */
    LAVA_CORRUPT_ROW, LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /* 57-60 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW,  /* 61-64 */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,   /* 65-68 */
    LAVA_ROCK_ROW, LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,  LAVA_ROW,        /* 69-72 */
    LAVA_CORRUPT_ROW, LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /* 73-76 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW,  /* 77-80 */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,   /* 81-84 */
    LAVA_ROCK_ROW, LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,  LAVA_ROW,        /* 85-88 */
    LAVA_CORRUPT_ROW, LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /* 89-92 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW,  /* 93-96 */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,   /* 97-100*/
    LAVA_ROCK_ROW, LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,  LAVA_ROW,        /*101-104*/
    LAVA_CORRUPT_ROW, LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /*105-108*/
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW,  /*109-112*/
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,   /*113-116*/
    LAVA_ROCK_ROW, LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,  LAVA_ROW,        /*117-120*/
    LAVA_CORRUPT_ROW, LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /*121-124*/
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW,  /*125-128*/
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,   /*129-132*/
    LAVA_ROCK_ROW, LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,  LAVA_ROW,        /*133-136*/
    LAVA_CORRUPT_ROW, LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /*137-140*/
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW,  /*141-144*/
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,   /*145-148*/
    LAVA_ROCK_ROW, LAVA_CORRUPT_ROW, LAVA_DIRT_ROW,  LAVA_ROW,        /*149-152*/
    LAVA_CORRUPT_ROW, LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /*153-156*/
    LAVA_DIRT_ROW, LAVA_ROW,                                            /*157-158*/
    TREE_ROW
};

/* =========================================================================
 * Section 8 — S-centre: lava sea with dirt ash paths and rock outcrops
 * ========================================================================= */
const u8 world_c_section8[160 * 160] = {
    TREE_ROW,
    LAVA_ROW,      LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_DIRT_ROW,    /*  1-4  */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_DIRT_ROW,  LAVA_ROW,         /*  5-8  */
    LAVA_ROCK_ROW, LAVA_DIRT_ROW,  LAVA_ROW,       LAVA_ROW,         /*  9-12 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROW,       LAVA_ROCK_ROW,    /* 13-16 */
    LAVA_ROW,      LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_DIRT_ROW,    /* 17-20 */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_DIRT_ROW,  LAVA_ROW,         /* 21-24 */
    LAVA_ROCK_ROW, LAVA_DIRT_ROW,  LAVA_ROW,       LAVA_ROW,         /* 25-28 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROW,       LAVA_ROCK_ROW,    /* 29-32 */
    LAVA_ROW,      LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_DIRT_ROW,    /* 33-36 */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_DIRT_ROW,  LAVA_ROW,         /* 37-40 */
    LAVA_ROCK_ROW, LAVA_DIRT_ROW,  LAVA_ROW,       LAVA_ROW,         /* 41-44 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROW,       LAVA_ROCK_ROW,    /* 45-48 */
    LAVA_ROW,      LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_DIRT_ROW,    /* 49-52 */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_DIRT_ROW,  LAVA_ROW,         /* 53-56 */
    LAVA_ROCK_ROW, LAVA_DIRT_ROW,  LAVA_ROW,       LAVA_ROW,         /* 57-60 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROW,       LAVA_ROCK_ROW,    /* 61-64 */
    LAVA_ROW,      LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_DIRT_ROW,    /* 65-68 */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_DIRT_ROW,  LAVA_ROW,         /* 69-72 */
    LAVA_ROCK_ROW, LAVA_DIRT_ROW,  LAVA_ROW,       LAVA_ROW,         /* 73-76 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROW,       LAVA_ROCK_ROW,    /* 77-80 */
    LAVA_ROW,      LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_DIRT_ROW,    /* 81-84 */
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_DIRT_ROW,  LAVA_ROW,         /* 85-88 */
    LAVA_ROCK_ROW, LAVA_DIRT_ROW,  LAVA_ROW,       LAVA_ROW,         /* 89-92 */
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROW,       LAVA_ROCK_ROW,    /* 93-96 */
    LAVA_ROW,      LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_DIRT_ROW,    /* 97-100*/
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_DIRT_ROW,  LAVA_ROW,         /*101-104*/
    LAVA_ROCK_ROW, LAVA_DIRT_ROW,  LAVA_ROW,       LAVA_ROW,         /*105-108*/
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROW,       LAVA_ROCK_ROW,    /*109-112*/
    LAVA_ROW,      LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_DIRT_ROW,    /*113-116*/
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_DIRT_ROW,  LAVA_ROW,         /*117-120*/
    LAVA_ROCK_ROW, LAVA_DIRT_ROW,  LAVA_ROW,       LAVA_ROW,         /*121-124*/
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROW,       LAVA_ROCK_ROW,    /*125-128*/
    LAVA_ROW,      LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_DIRT_ROW,    /*129-132*/
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_DIRT_ROW,  LAVA_ROW,         /*133-136*/
    LAVA_ROCK_ROW, LAVA_DIRT_ROW,  LAVA_ROW,       LAVA_ROW,         /*137-140*/
    LAVA_DIRT_ROW, LAVA_ROW,       LAVA_ROW,       LAVA_ROCK_ROW,    /*141-144*/
    LAVA_ROW,      LAVA_ROW,       LAVA_ROCK_ROW,  LAVA_DIRT_ROW,    /*145-148*/
    LAVA_ROW,      LAVA_ROCK_ROW,  LAVA_DIRT_ROW,  LAVA_ROW,         /*149-152*/
    LAVA_ROCK_ROW, LAVA_DIRT_ROW,  LAVA_ROW,       LAVA_ROW,         /*153-156*/
    LAVA_DIRT_ROW, LAVA_ROW,                                           /*157-158*/
    TREE_ROW
};

/* =========================================================================
 * Section 9 — SE: dense lava sea with corruption patches and rock clusters
 * ========================================================================= */
const u8 world_c_section9[160 * 160] = {
    TREE_ROW,
    LAVA_ROW,      LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /*  1-4  */
    LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW, LAVA_ROW,        /*  5-8  */
    LAVA_ROW,      LAVA_ROCK_ROW,    LAVA_ROW,       LAVA_CORRUPT_ROW,/*  9-12 */
    LAVA_ROCK_ROW, LAVA_ROW,         LAVA_CORRUPT_ROW, LAVA_ROW,      /* 13-16 */
    LAVA_ROW,      LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /* 17-20 */
    LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW, LAVA_ROW,        /* 21-24 */
    LAVA_ROW,      LAVA_ROCK_ROW,    LAVA_ROW,       LAVA_CORRUPT_ROW,/* 25-28 */
    LAVA_ROCK_ROW, LAVA_ROW,         LAVA_CORRUPT_ROW, LAVA_ROW,      /* 29-32 */
    LAVA_ROW,      LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /* 33-36 */
    LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW, LAVA_ROW,        /* 37-40 */
    LAVA_ROW,      LAVA_ROCK_ROW,    LAVA_ROW,       LAVA_CORRUPT_ROW,/* 41-44 */
    LAVA_ROCK_ROW, LAVA_ROW,         LAVA_CORRUPT_ROW, LAVA_ROW,      /* 45-48 */
    LAVA_ROW,      LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /* 49-52 */
    LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW, LAVA_ROW,        /* 53-56 */
    LAVA_ROW,      LAVA_ROCK_ROW,    LAVA_ROW,       LAVA_CORRUPT_ROW,/* 57-60 */
    LAVA_ROCK_ROW, LAVA_ROW,         LAVA_CORRUPT_ROW, LAVA_ROW,      /* 61-64 */
    LAVA_ROW,      LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /* 65-68 */
    LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW, LAVA_ROW,        /* 69-72 */
    LAVA_ROW,      LAVA_ROCK_ROW,    LAVA_ROW,       LAVA_CORRUPT_ROW,/* 73-76 */
    LAVA_ROCK_ROW, LAVA_ROW,         LAVA_CORRUPT_ROW, LAVA_ROW,      /* 77-80 */
    LAVA_ROW,      LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /* 81-84 */
    LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW, LAVA_ROW,        /* 85-88 */
    LAVA_ROW,      LAVA_ROCK_ROW,    LAVA_ROW,       LAVA_CORRUPT_ROW,/* 89-92 */
    LAVA_ROCK_ROW, LAVA_ROW,         LAVA_CORRUPT_ROW, LAVA_ROW,      /* 93-96 */
    LAVA_ROW,      LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /* 97-100*/
    LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW, LAVA_ROW,        /*101-104*/
    LAVA_ROW,      LAVA_ROCK_ROW,    LAVA_ROW,       LAVA_CORRUPT_ROW,/*105-108*/
    LAVA_ROCK_ROW, LAVA_ROW,         LAVA_CORRUPT_ROW, LAVA_ROW,      /*109-112*/
    LAVA_ROW,      LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /*113-116*/
    LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW, LAVA_ROW,        /*117-120*/
    LAVA_ROW,      LAVA_ROCK_ROW,    LAVA_ROW,       LAVA_CORRUPT_ROW,/*121-124*/
    LAVA_ROCK_ROW, LAVA_ROW,         LAVA_CORRUPT_ROW, LAVA_ROW,      /*125-128*/
    LAVA_ROW,      LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /*129-132*/
    LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW, LAVA_ROW,        /*133-136*/
    LAVA_ROW,      LAVA_ROCK_ROW,    LAVA_ROW,       LAVA_CORRUPT_ROW,/*137-140*/
    LAVA_ROCK_ROW, LAVA_ROW,         LAVA_CORRUPT_ROW, LAVA_ROW,      /*141-144*/
    LAVA_ROW,      LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW,   /*145-148*/
    LAVA_CORRUPT_ROW, LAVA_ROW,       LAVA_ROCK_ROW, LAVA_ROW,        /*149-152*/
    LAVA_ROW,      LAVA_ROCK_ROW,    LAVA_ROW,       LAVA_CORRUPT_ROW,/*153-156*/
    LAVA_ROCK_ROW, LAVA_ROW,                                            /*157-158*/
    TREE_ROW
};

#undef G
#undef D
#undef W
#undef T
#undef R
#undef L
#undef I
#undef C
