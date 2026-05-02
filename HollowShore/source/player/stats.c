#include "stats.h"

/* ---- Internal frame counters (static, persist across calls) -------------- */
static u16 s_hunger_frame  = 0;
static u16 s_stamina_frame = 0;
static u16 s_starve_frame  = 0;

/* ---- stats_init ---------------------------------------------------------- */
void stats_init(PlayerStats *s)
{
    s->hp          = 100;
    s->max_hp      = 100;
    s->stamina     = 200;
    s->max_stamina = 200;
    s->hunger      = 300;
    s->max_hunger  = 300;

    s_hunger_frame  = 0;
    s_stamina_frame = 0;
    s_starve_frame  = 0;
}

/* ---- stamina_update ------------------------------------------------------ */
/*
 * Call once per frame.
 *   sprinting | using_tool  → drain 1 per frame
 *   near_bed (resting)      → recover 3 per frame
 *   otherwise               → recover 1 every 2 frames
 */
void stamina_update(PlayerStats *s, bool sprinting, bool using_tool, bool near_bed)
{
    if (sprinting || using_tool)
    {
        if (s->stamina > 0)
            s->stamina--;
        s_stamina_frame = 0;
    }
    else if (near_bed)
    {
        s->stamina += 3;
        if (s->stamina > s->max_stamina)
            s->stamina = s->max_stamina;
        s_stamina_frame = 0;
    }
    else
    {
        /* recover 1 every 2 frames */
        s_stamina_frame++;
        if (s_stamina_frame >= 2)
        {
            s_stamina_frame = 0;
            if (s->stamina < s->max_stamina)
                s->stamina++;
        }
    }
}

/* ---- hunger_update ------------------------------------------------------- */
/*
 * Call once per frame.
 *   is_active (moving/attacking) → drain 1 every 120 frames
 *   otherwise                    → drain 1 every 240 frames
 *   STARVING (hunger <= 0)       → lose 1 HP every 60 frames
 */
void hunger_update(PlayerStats *s, bool is_active)
{
    /* Hunger drain */
    s_hunger_frame++;
    {
        u16 threshold = is_active ? 120 : 240;
        if (s_hunger_frame >= threshold)
        {
            s_hunger_frame = 0;
            if (s->hunger > 0)
                s->hunger--;
        }
    }

    /* Starving HP drain */
    if (s->hunger <= 0)
    {
        s->hunger = 0;
        s_starve_frame++;
        if (s_starve_frame >= 60)
        {
            s_starve_frame = 0;
            if (s->hp > 0)
                s->hp--;
        }
    }
    else
    {
        s_starve_frame = 0;
    }
}

/* ---- hunger_stage -------------------------------------------------------- */
/*
 *  FULL      >=  75% of max_hunger
 *  SATISFIED >=  40%
 *  HUNGRY    >=  10%
 *  STARVING  <   10%
 */
HungerStage hunger_stage(const PlayerStats *s)
{
    /* Use integer arithmetic to avoid floats: multiply threshold by 100,
       compare against (hunger * 100 / max_hunger). */
    s16 pct;

    if (s->max_hunger == 0)
        return HUNGER_STARVING;

    pct = (s16)((s->hunger * 100) / s->max_hunger);

    if (pct >= 75)
        return HUNGER_FULL;
    if (pct >= 40)
        return HUNGER_SATISFIED;
    if (pct >= 10)
        return HUNGER_HUNGRY;
    return HUNGER_STARVING;
}
