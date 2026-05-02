/*
 * palette.c — BG and sprite palette management with time-of-day and weather tinting.
 *
 * GBA palette RAM layout:
 *   0x05000000  BG palette  (256 entries × 16 palette banks, each u16)
 *   0x05000200  OBJ palette (same layout)
 *
 * Each palette bank holds 16 colours; pal_index selects which bank.
 * Fixed-point tinting uses a simple 5-bit per-channel multiply:
 *   tinted = (original_channel * factor) >> 8   (factor 256 == 1.0)
 */

#include "palette.h"
#include "../gba.h"

/* ---- Constants ----------------------------------------------------------- */
#define OBJ_PAL_BASE  ((vu16*)0x05000200)
#define COLOURS_PER_PAL  16

/* ---- BG palette base reference ------------------------------------------- */
/* Stores the untinted BG palette so tinting functions always start from the
   canonical colours rather than stacking tints on previous tints. */
static u16 base_bg_pal[256];

/* ---- Internal helpers ---------------------------------------------------- */

/* Extract individual 5-bit RGB channels from a packed GBA colour. */
static inline u8 col_r(u16 c) { return (u8)( c        & 0x1F); }
static inline u8 col_g(u16 c) { return (u8)((c >>  5) & 0x1F); }
static inline u8 col_b(u16 c) { return (u8)((c >> 10) & 0x1F); }

/* Pack 5-bit channels (clamped to 0–31) back into a GBA colour. */
static inline u16 col_pack(u8 r, u8 g, u8 b)
{
    return RGB15(r, g, b);
}

/* Multiply a 5-bit channel value by a fixed-point factor and clamp.
   factor is in 8-bit fixed point: 256 == 1.0, 128 == 0.5, 320 == 1.25 ... */
static inline u8 channel_mul(u8 ch, u16 factor)
{
    u16 result = ((u16)ch * factor) >> 8;
    return (result > 31) ? 31 : (u8)result;
}

/* Add a bias to a channel and clamp to [0, 31]. */
static inline u8 channel_add(u8 ch, s16 bias)
{
    s16 result = (s16)ch + bias;
    if (result < 0)  return 0;
    if (result > 31) return 31;
    return (u8)result;
}

/* Apply a per-channel (r_f, g_f, b_f) factor (fixed-point 256==1.0) plus
   optional bias to a single source colour, returning the tinted result.
   Channel 0 (transparent) is always left at 0. */
static u16 tint_colour(u16 src, u16 r_f, u16 g_f, u16 b_f,
                        s16 r_b, s16 g_b, s16 b_b)
{
    if (src == 0) return 0; /* transparent entry — never touch */
    u8 r = channel_add(channel_mul(col_r(src), r_f), r_b);
    u8 g = channel_add(channel_mul(col_g(src), g_f), g_b);
    u8 b = channel_add(channel_mul(col_b(src), b_f), b_b);
    return col_pack(r, g, b);
}

/* Write 16 u16 entries from src[] to dest[] using volatile word writes.
   Using individual u16 writes is safe in all GBA contexts. */
static void pal_copy16(vu16 *dest, const u16 *src)
{
    u8 i;
    for (i = 0; i < COLOURS_PER_PAL; ++i)
        dest[i] = src[i];
}

/* ---- Public API ---------------------------------------------------------- */

/*
 * load_bg_palette — copy 16 colours into BG palette bank pal_index.
 * Also stores them into base_bg_pal so tinting starts from the right base.
 */
void load_bg_palette(const u16 *pal, u8 pal_index)
{
    vu16 *dest = MEM_PALETTE + (u16)pal_index * COLOURS_PER_PAL;
    u16  *base = base_bg_pal + (u16)pal_index * COLOURS_PER_PAL;
    u8 i;
    for (i = 0; i < COLOURS_PER_PAL; ++i) {
        dest[i] = pal[i];
        base[i] = pal[i];
    }
}

/*
 * load_sprite_palette — copy 16 colours into OBJ palette bank pal_index.
 */
void load_sprite_palette(const u16 *pal, u8 pal_index)
{
    vu16 *dest = OBJ_PAL_BASE + (u16)pal_index * COLOURS_PER_PAL;
    u8 i;
    for (i = 0; i < COLOURS_PER_PAL; ++i)
        dest[i] = pal[i];
}

/*
 * palette_apply_time_of_day — tint the full BG palette (all 256 entries)
 * based on the current day phase and hour.
 *
 * Phase   Effect
 * ------  -------------------------------------------------------
 * DAY     Identity (256/256 per channel)
 * DAWN    Warm orange: boost red, slight green cut, cut blue
 *         Intensity fades as hour moves 5→7 (dawn → full day)
 * DUSK    Same warm tint as dawn (hours 17→19)
 * NIGHT   Dark blue: reduce all channels, slight blue boost
 */
void palette_apply_time_of_day(DayPhase phase, u8 hour)
{
    /* Default: identity */
    u16 r_f = 256, g_f = 256, b_f = 256;
    s16 r_b = 0,   g_b = 0,   b_b = 0;

    switch (phase) {
    case PHASE_DAWN:
        /*
         * Dawn is roughly hours 5–7.
         * At hour 5 use strong warm tint; at hour 7 approach identity.
         * Blend factor 0–4 maps hour 5 to 0, hour 7+ to 4.
         */
        {
            u8 blend = (hour >= 7) ? 4 : (u8)(hour - 5); /* 0,1,2,3,4 */
            /* r_f: 330 → 256 over blend 0→4 */
            r_f = (u16)(330 - (u16)blend * 18);
            /* g_f: 230 → 256 over blend 0→4 */
            g_f = (u16)(230 + (u16)blend * 6);
            /* b_f: 160 → 256 over blend 0→4 */
            b_f = (u16)(160 + (u16)blend * 24);
        }
        break;

    case PHASE_DUSK:
        /*
         * Dusk is roughly hours 17–19.
         * At hour 17 approaching evening; at 19 fully dark.
         */
        {
            u8 blend = (hour <= 17) ? 0 : (u8)(hour - 17); /* 0,1,2 */
            if (blend > 4) blend = 4;
            /* Start near identity at hour 17, deepen toward night at 19+ */
            r_f = (u16)(320 - (u16)blend * 16);
            g_f = (u16)(220 + (u16)blend * 4);
            b_f = (u16)(150 + (u16)blend * 20);
        }
        break;

    case PHASE_NIGHT:
        /* Dark blue overlay: 55% brightness, slight blue lift */
        r_f = 140;
        g_f = 140;
        b_f = 160;
        break;

    case PHASE_DAY:
    default:
        /* Full identity — restore base palette unchanged */
        break;
    }

    /* Apply tint to all 256 entries */
    {
        u16 i;
        for (i = 0; i < 256; ++i) {
            MEM_PALETTE[i] = tint_colour(base_bg_pal[i],
                                         r_f, g_f, b_f,
                                         r_b, g_b, b_b);
        }
    }
}

/*
 * palette_apply_weather — overlay a weather tint on top of the current
 * time-of-day palette already in VRAM.  Reads from what is currently in
 * MEM_PALETTE so it stacks on the time-of-day tint.
 *
 * Weather      Effect
 * ---------    ---------------------------------------------------
 * CLEAR        No change
 * RAIN         Desaturate slightly, darken (move channels toward grey)
 * STORM        Heavier darken + strong desaturate + slight blue
 * BLIZZARD     Shift toward cold blue-white (boost all, bias toward blue)
 */
void palette_apply_weather(WeatherState weather)
{
    u16 i;
    u16 r_f, g_f, b_f;
    s16 r_b, g_b, b_b;

    switch (weather) {
    case WEATHER_RAIN:
        r_f = 210; g_f = 215; b_f = 230;
        r_b = 0;   g_b = 0;   b_b = 1;
        break;

    case WEATHER_STORM:
        r_f = 170; g_f = 175; b_f = 200;
        r_b = 0;   g_b = 0;   b_b = 2;
        break;

    case WEATHER_BLIZZARD:
        /* Brighten everything but bias toward blue-white */
        r_f = 230; g_f = 235; b_f = 270;
        r_b = 4;   g_b = 4;   b_b = 6;
        break;

    case WEATHER_CLEAR:
    default:
        return; /* nothing to do */
    }

    for (i = 0; i < 256; ++i) {
        u16 src = MEM_PALETTE[i];
        if (src == 0) continue; /* transparent */
        MEM_PALETTE[i] = tint_colour(src,
                                     r_f, g_f, b_f,
                                     r_b, g_b, b_b);
    }
}
