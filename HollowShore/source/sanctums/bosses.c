#include "bosses.h"

/* ---- Global active boss state (extern declared in bosses.h) ------------- */
Boss active_boss;

/* ---- boss_take_hit -------------------------------------------------------
   Subtract damage from active_boss.hp.
   Trigger phase 2 transition when HP falls to or below 50% for the first
   time (phase == 0).  Clamp HP to zero so it never goes negative.
--------------------------------------------------------------------------- */
void boss_take_hit(u8 damage)
{
    if (!active_boss.active)
        return;

    active_boss.hp -= (s16)damage;

    /* Phase transition: 50% HP threshold, only fires once (phase 0 -> 1) */
    if (active_boss.phase == 0 &&
        active_boss.hp <= active_boss.max_hp / 2)
    {
        active_boss.phase = 1;
    }

    /* Clamp to zero */
    if (active_boss.hp < 0)
        active_boss.hp = 0;
}

/* ---- boss_is_defeated ----------------------------------------------------
   Returns TRUE when the boss HP has been drained to zero.
--------------------------------------------------------------------------- */
bool boss_is_defeated(void)
{
    return (active_boss.hp <= 0) ? TRUE : FALSE;
}
