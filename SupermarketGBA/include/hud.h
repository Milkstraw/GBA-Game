#ifndef HUD_SYSTEM_H
#define HUD_SYSTEM_H

#include "gba_types.h"

/*
 * HUD lives on BG0, top row of 8×8 tiles (y=0).
 * BG0 uses charblock 0, screenblock 7 (TE_SBB_BG0=7).
 *
 * Tile glyph layout in charblock 0:
 *   Tiles 0–127: game tileset (reserved for future graphics)
 *   Tiles 128+:  HUD font glyphs (digits, letters, punctuation)
 *
 * To avoid colliding with game tiles we place glyphs at tile 128+:
 *   128 = space
 *   129 = '0'  130='1' ... 138='9'
 *   139 = 'A'  ... 164='Z'
 *   165 = '$'  166='.'  167=':'  168='/'  169='+'  170='-'
 */
#define HUD_GLYPH_BASE   128u  /* first glyph tile index in charblock 0 */
#define HUD_GLYPH_SPACE  128u
#define HUD_GLYPH_DIGIT0 129u  /* '0'–'9' at 129–138 */
#define HUD_GLYPH_A      139u  /* 'A'–'Z' at 139–164 */
#define HUD_GLYPH_DOLLAR 165u
#define HUD_GLYPH_DOT    166u
#define HUD_GLYPH_COLON  167u
#define HUD_GLYPH_SLASH  168u
#define HUD_GLYPH_PLUS   169u
#define HUD_GLYPH_MINUS  170u

/* Number of glyph tiles uploaded to VRAM */
#define HUD_NUM_GLYPHS   43u   /* space + 10 digits + 26 letters + 6 punctuation */

/* Screen width in 8-pixel tiles */
#define HUD_TILE_COLS    30u

/* Initialise BG0 for HUD display; upload font tiles into charblock 0. */
void hud_init(void);

/* Redraw all four HUD zones from live game state (call every frame). */
void hud_draw(void);

#endif /* HUD_SYSTEM_H */
