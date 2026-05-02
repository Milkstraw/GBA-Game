/*
 * transition.c — Section-transition fade state machine for HollowShore.
 *
 * State sequence:
 *   TRANS_IDLE
 *     -> TRANS_FADING_OUT  (player reached a section edge)
 *     -> TRANS_LOADING     (fade complete; load next section)
 *     -> TRANS_FADING_IN   (section ready; fade back in)
 *     -> TRANS_IDLE
 *
 * Palette fade strategy:
 *   We maintain a shadow copy of all 256 BG palette entries captured at
 *   fade-out start.  During FADING_OUT we scale each colour component
 *   towards 0 using an integer brightness step (0 = black … 16 = full).
 *   During FADING_IN we scale back up to the saved values.
 *   One palette update per vsync tick; the fade runs for FADE_FRAMES frames.
 *
 * GBA palette register layout (RGB15):
 *   bits  4: 0  = Red   (0–31)
 *   bits  9: 5  = Green (0–31)
 *   bits 14:10  = Blue  (0–31)
 */

#include "transition.h"
#include "section.h"           /* section_id_from_direction, SECTION_TILES_W/H, TILE_SIZE */
#include "../gba.h"            /* MEM_PALETTE, vsync, REG_VCOUNT */

/* Number of frames for a full fade-out or fade-in */
#define FADE_FRAMES  30u

/* Number of BG palette entries to fade (full 256-colour BG palette) */
#define PALETTE_SIZE 256u

/* Section pixel dimensions */
#define SECTION_PX_W  ((s16)((s16)SECTION_TILES_W * (s16)TILE_SIZE))
#define SECTION_PX_H  ((s16)((s16)SECTION_TILES_H * (s16)TILE_SIZE))

/* -------------------------------------------------------------------------- */
/* Module state                                                                */
/* -------------------------------------------------------------------------- */

static TransitionState s_state            = TRANS_IDLE;
static u8              s_fade_counter     = 0;
static u8              s_pending_id       = 0;
static u8              s_direction        = DIR_UP; /* direction that triggered */
static u16             s_saved_palette[PALETTE_SIZE];

/* -------------------------------------------------------------------------- */
/* Internal helpers                                                            */
/* -------------------------------------------------------------------------- */

/* Save a copy of the current BG palette so we can restore it during fade-in. */
static void palette_save(void)
{
    u32 i;
    for (i = 0; i < PALETTE_SIZE; ++i) {
        s_saved_palette[i] = MEM_PALETTE[i];
    }
}

/*
 * Apply a brightness level to the palette.
 *   level = 16 → full brightness (original colours)
 *   level =  0 → all black
 *
 * Each RGB component is scaled: new = (orig * level) >> 4
 */
static void palette_apply_brightness(u8 level)
{
    u32 i;
    for (i = 0; i < PALETTE_SIZE; ++i) {
        u16 c   = s_saved_palette[i];
        u16 r   = (u16)((c & 0x001Fu));
        u16 g   = (u16)((c & 0x03E0u) >> 5u);
        u16 b   = (u16)((c & 0x7C00u) >> 10u);

        r = (u16)((r * (u16)level) >> 4u);
        g = (u16)((g * (u16)level) >> 4u);
        b = (u16)((b * (u16)level) >> 4u);

        MEM_PALETTE[i] = (u16)(r | (g << 5u) | (b << 10u));
    }
}

/* Write the fully-black palette (fast path at the start of FADING_OUT). */
static void palette_blackout(void)
{
    u32 i;
    for (i = 0; i < PALETTE_SIZE; ++i) {
        MEM_PALETTE[i] = 0x0000u;
    }
}

/* Restore the saved palette to full brightness. */
static void palette_restore(void)
{
    u32 i;
    for (i = 0; i < PALETTE_SIZE; ++i) {
        MEM_PALETTE[i] = s_saved_palette[i];
    }
}

/* Determine which direction the player exited and start the fade. */
static void begin_transition(s16 player_x, s16 player_y, u8 current_id,
                              u8 dir)
{
    s_direction   = dir;
    s_pending_id  = section_id_from_direction(current_id, dir);
    s_fade_counter = 0;

    /* Only transition if there is an adjacent section */
    if (s_pending_id == current_id) {
        return; /* at the world boundary — do nothing */
    }

    palette_save();
    s_state = TRANS_FADING_OUT;

    /* Suppress unused-parameter warnings in minimal builds */
    (void)player_x;
    (void)player_y;
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

/*
 * transition_check — call once per frame while TRANS_IDLE.
 *
 * Player position is in pixels relative to the current section's origin.
 * A section spans [0, SECTION_PX_W) × [0, SECTION_PX_H).
 * Crossing any edge triggers the corresponding directional transition.
 */
void transition_check(s16 player_x, s16 player_y, u8 current_section_id)
{
    if (s_state != TRANS_IDLE) {
        return; /* already in progress */
    }

    if (player_x < 0) {
        begin_transition(player_x, player_y, current_section_id, (u8)DIR_LEFT);
    } else if (player_x >= SECTION_PX_W) {
        begin_transition(player_x, player_y, current_section_id, (u8)DIR_RIGHT);
    } else if (player_y < 0) {
        begin_transition(player_x, player_y, current_section_id, (u8)DIR_UP);
    } else if (player_y >= SECTION_PX_H) {
        begin_transition(player_x, player_y, current_section_id, (u8)DIR_DOWN);
    }
}

/*
 * transition_update — advance the state machine by one frame.
 * Call once per vsync (after vsync() returns).
 */
void transition_update(void)
{
    switch (s_state) {
    case TRANS_IDLE:
        /* Nothing to do */
        break;

    case TRANS_FADING_OUT:
        /*
         * Darken the palette over FADE_FRAMES frames.
         * brightness goes from 16 down to 0.
         */
        {
            u8 brightness;
            ++s_fade_counter;
            if (s_fade_counter >= (u8)FADE_FRAMES) {
                palette_blackout();
                s_fade_counter = 0;
                s_state = TRANS_LOADING;
            } else {
                /* Map counter [0..FADE_FRAMES-1] → brightness [16..1] */
                brightness = (u8)(16u - (u8)((u32)s_fade_counter * 16u /
                                              (u32)FADE_FRAMES));
                palette_apply_brightness(brightness);
            }
        }
        break;

    case TRANS_LOADING:
        /*
         * The palette is now black.  The actual section_load() call must be
         * driven by the game loop (which holds the Section buffer); we simply
         * expose s_pending_id via transition_get_state() == TRANS_LOADING so
         * the game loop knows to reload.  After one frame the game loop should
         * have already loaded the new section, so we proceed to FADING_IN.
         *
         * palette_save() is NOT called here because we want to fade in from
         * the new section's palette, which hasn't changed (same palette ROM).
         * The saved palette from the fade-out is still valid.
         */
        s_state = TRANS_FADING_IN;
        s_fade_counter = 0;
        break;

    case TRANS_FADING_IN:
        /*
         * Brighten the palette over FADE_FRAMES frames.
         * brightness goes from 1 up to 16.
         */
        {
            u8 brightness;
            ++s_fade_counter;
            if (s_fade_counter >= (u8)FADE_FRAMES) {
                palette_restore();
                s_fade_counter = 0;
                s_state = TRANS_IDLE;
            } else {
                /* Map counter [0..FADE_FRAMES-1] → brightness [1..15] */
                brightness = (u8)(1u + (u8)((u32)s_fade_counter * 16u /
                                             (u32)FADE_FRAMES));
                if (brightness > 16u) { brightness = 16u; }
                palette_apply_brightness(brightness);
            }
        }
        break;

    default:
        s_state = TRANS_IDLE;
        break;
    }
}

TransitionState transition_get_state(void)
{
    return s_state;
}
