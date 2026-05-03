#ifndef LEVELS_H
#define LEVELS_H

#include "gba.h"

/* Brick type codes used in layout tables */
#define BT_EMPTY 0   /* no brick */
#define BT_1HIT  1   /* 1 hit to destroy */
#define BT_2HIT  2   /* 2 hits */
#define BT_3HIT  3   /* 3 hits */
#define BT_STEEL 4   /* indestructible */

#define NUM_LEVELS 8

/*
 * Layout[level][row][col] — row 0 is the TOP row (y=14), row 5 is the
 * BOTTOM row nearest the paddle.  Each layout is 6 rows × 8 columns.
 *
 * Design rule: every BT_1/2/3 brick must be reachable by the ball
 * (no completely steel-enclosed pockets).
 */
static const u8 LEVEL_LAYOUTS[NUM_LEVELS][6][8] = {

    /* ---- Level 1: Classic full grid ---- */
    {{1,1,1,1,1,1,1,1},
     {1,1,1,1,1,1,1,1},
     {1,1,1,1,1,1,1,1},
     {1,1,1,1,1,1,1,1},
     {1,1,1,1,1,1,1,1},
     {1,1,1,1,1,1,1,1}},

    /* ---- Level 2: Checkerboard ---- */
    {{1,0,1,0,1,0,1,0},
     {0,1,0,1,0,1,0,1},
     {1,0,1,0,1,0,1,0},
     {0,1,0,1,0,1,0,1},
     {1,0,1,0,1,0,1,0},
     {0,1,0,1,0,1,0,1}},

    /* ---- Level 3: Diamond (2-hit core) ---- */
    {{0,0,0,1,1,0,0,0},
     {0,0,1,1,1,1,0,0},
     {0,1,1,2,2,1,1,0},
     {0,1,1,2,2,1,1,0},
     {0,0,1,1,1,1,0,0},
     {0,0,0,1,1,0,0,0}},

    /* ---- Level 4: Cross / plus (all 2-hit) ---- */
    {{0,0,2,2,2,2,0,0},
     {0,0,2,2,2,2,0,0},
     {2,2,2,2,2,2,2,2},
     {2,2,2,2,2,2,2,2},
     {0,0,2,2,2,2,0,0},
     {0,0,2,2,2,2,0,0}},

    /* ---- Level 5: Fortress
     *   Steel border + tiered interior.
     *   Row 5 is 1-hit so the ball can enter; rows 2-4 are accessible
     *   once the bottom row is cleared.                                ---- */
    {{4,4,4,4,4,4,4,4},
     {4,0,0,0,0,0,0,4},
     {4,0,2,2,2,2,0,4},
     {4,0,2,3,3,2,0,4},
     {4,0,2,2,2,2,0,4},
     {1,1,1,1,1,1,1,1}},

    /* ---- Level 6: Z-stripe (fast, sparse) ---- */
    {{1,1,1,1,1,1,1,1},
     {0,0,0,0,0,2,0,0},
     {0,0,0,0,2,0,0,0},
     {0,0,0,2,0,0,0,0},
     {0,0,2,0,0,0,0,0},
     {1,1,1,1,1,1,1,1}},

    /* ---- Level 7: Steel pillars with 2-hit interior ---- */
    {{1,4,1,1,1,1,4,1},
     {1,4,1,1,1,1,4,1},
     {1,4,2,2,2,2,4,1},
     {1,4,2,2,2,2,4,1},
     {1,4,1,1,1,1,4,1},
     {1,4,1,1,1,1,4,1}},

    /* ---- Level 8: Gauntlet (3-hit bricks, steel obstacles)
     *   Row 3 is an open channel so the ball can reach top rows.      ---- */
    {{3,4,3,3,3,3,4,3},
     {3,4,3,4,4,3,4,3},
     {3,4,3,4,4,3,4,3},
     {3,0,0,0,0,0,0,3},
     {3,3,4,3,3,4,3,3},
     {3,3,3,3,3,3,3,3}},
};

/* Per-level row colour palettes: LEVEL_COLORS[level][row] */
static const u16 LEVEL_COLORS[NUM_LEVELS][6] = {
    /* Level 1: classic rainbow */
    {0x001F,0x021F,0x039F,0x0380,0x7B00,0x7C00},
    /* Level 2: same */
    {0x001F,0x021F,0x039F,0x0380,0x7B00,0x7C00},
    /* Level 3: purple hues */
    {0x7C1F,0x601F,0x401F,0x401F,0x601F,0x7C1F},
    /* Level 4: blue-green */
    {0x03E0,0x03C8,0x03A0,0x0390,0x0370,0x0350},
    /* Level 5: orange-red */
    {0x001F,0x020F,0x039E,0x039E,0x020F,0x001F},
    /* Level 6: cyan-white */
    {0x7FFF,0x6B5A,0x56B5,0x4210,0x56B5,0x7FFF},
    /* Level 7: warm gold */
    {0x03FF,0x03DF,0x03BF,0x039F,0x037F,0x035F},
    /* Level 8: vivid mix */
    {0x001F,0x7C00,0x03E0,0x7FE0,0x7C1F,0x03FF},
};

#endif /* LEVELS_H */
