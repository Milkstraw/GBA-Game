#include "sanctum.h"
#include "bosses.h"
#include "roothold.h"
#include "tidecrypt.h"
#include "frostspire.h"
#include "marshveil.h"
#include "emberdepth.h"
#include "hollow_throne.h"
#include "../gba.h"

/* ---- Module state ------------------------------------------------------- */
static Sanctum current_sanctum;
static bool    in_sanctum = FALSE;

/* ---- Screenblock base for BG0 dungeon tiles ----------------------------- */
/* GBA: each screenblock = 2 KB (0x800 bytes). Screenblock 8 = 0x06004000.
   Charblock 0 occupies screenblocks 0-7 (tiles); screenblock 8 is the first
   map block that does not overlap charblock 0 data.                         */
#define DUNGEON_SCREENBLOCK  ((vu16*)0x06004000)

/* BG0CNT: priority 0, charblock 0, screenblock 8 (bits[12:8]=8), 256-col   */
/* bit layout: [1:0]=priority(0), [3:2]=charblock(0), [8]=256col, [12:8]=sb */
#define DUNGEON_BG0CNT_VAL  ((u16)((8 << 8) | (0 << 2)))

/* ---- Boss type per sanctum ID (IDs are 1-based) ------------------------- */
/* Mapping: 1=Root Warden, 2=Drowned Warden, 3=Glacial Warden,
            4=Bog Specter,  5=Ember Titan,    6=Hollow Sovereign           */
static const BossType s_boss_type_for_sanctum[SANCTUM_COUNT] = {
    BOSS_ROOT_WARDEN,      /* sanctum id 1 */
    BOSS_DROWNED_WARDEN,   /* sanctum id 2 */
    BOSS_GLACIAL_WARDEN,   /* sanctum id 3 */
    BOSS_BOG_SPECTER,      /* sanctum id 4 */
    BOSS_EMBER_TITAN,      /* sanctum id 5 */
    BOSS_HOLLOW_SOVEREIGN  /* sanctum id 6 */
};

/* Default HP per boss (used when spawning; individual inits may override) */
static const s16 s_boss_max_hp[SANCTUM_COUNT] = {
    200,  /* Root Warden      */
    200,  /* Drowned Warden   */
    200,  /* Glacial Warden   */
    220,  /* Bog Specter      */
    240,  /* Ember Titan      */
    300   /* Hollow Sovereign */
};

/* ---- spawn_boss_for_sanctum ----------------------------------------------
   Initialise active_boss for the sanctum at sanctum_id (1-based).
   The boss starts at the centre of the 30×20 boss floor (pixels: 120, 80),
   with phase 0 and active = TRUE.
--------------------------------------------------------------------------- */
static void spawn_boss_for_sanctum(u8 sanctum_id)
{
    u8 idx = (u8)(sanctum_id - 1u);  /* convert to 0-based index */
    if (idx >= SANCTUM_COUNT)
        return;

    active_boss.type      = s_boss_type_for_sanctum[idx];
    active_boss.x         = (DUNGEON_FLOOR_W / 2) * 8;  /* tile-centre → pixel */
    active_boss.y         = (DUNGEON_FLOOR_H / 2) * 8;
    active_boss.max_hp    = s_boss_max_hp[idx];
    active_boss.hp        = s_boss_max_hp[idx];
    active_boss.phase     = 0;
    active_boss.active    = TRUE;
    active_boss.sprite_id = (u8)(64u + idx);  /* sprite slot offset per boss */
}

/* ---- sanctum_enter -------------------------------------------------------
   Configure the current sanctum, call the appropriate sanctum-specific init
   to populate floor tile data, then load floor 0.
--------------------------------------------------------------------------- */
void sanctum_enter(u8 sanctum_id)
{
    current_sanctum.id            = sanctum_id;
    current_sanctum.current_floor = 0;
    current_sanctum.boss_defeated = FALSE;

    /* Dispatch to the appropriate sanctum initialiser */
    switch (sanctum_id)
    {
        case 1: roothold_init(&current_sanctum);      break;
        case 2: tidecrypt_init(&current_sanctum);     break;
        case 3: frostspire_init(&current_sanctum);    break;
        case 4: marshveil_init(&current_sanctum);     break;
        case 5: emberdepth_init(&current_sanctum);    break;
        case 6: hollow_throne_init(&current_sanctum); break;
        default: break;
    }

    in_sanctum = TRUE;

    /* Load the first floor */
    sanctum_floor_load(&current_sanctum, 0);
}

/* ---- sanctum_floor_load --------------------------------------------------
   Write the 30×20 tile array of floor[floor_index] into the BG0 screenblock
   at 0x06004000.  GBA screenblocks are 32 entries wide, so each row occupies
   32 u16 entries (the extra two are unused and left as-is).

   When loading the boss floor (index 2) and the boss has not been defeated,
   spawn active_boss.
--------------------------------------------------------------------------- */
void sanctum_floor_load(Sanctum *s, u8 floor_index)
{
    u8  row, col;
    u16 tile_entry;

    if (floor_index >= DUNGEON_FLOORS)
        return;

    s->current_floor = floor_index;

    /* Configure BG0 for dungeon rendering (mode 0, tiled) */
    REG_BG0CNT = DUNGEON_BG0CNT_VAL;

    /* Reset scroll to top-left of the dungeon floor */
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;

    /* Write tile indices into BG0 screenblock (screenblock 1 = 0x06004000) */
    for (row = 0; row < DUNGEON_FLOOR_H; row++)
    {
        for (col = 0; col < DUNGEON_FLOOR_W; col++)
        {
            tile_entry = (u16)s->floors[floor_index].tiles[row * DUNGEON_FLOOR_W + col];
            DUNGEON_SCREENBLOCK[(u16)row * 32u + (u16)col] = tile_entry;
        }
    }

    /* Enable BG0 display */
    REG_DISPCNT |= BG0_ENABLE;

    /* Spawn boss on the boss floor if not yet defeated */
    if (floor_index == 2 && !s->boss_defeated)
    {
        spawn_boss_for_sanctum(s->id);
        REG_DISPCNT |= OBJ_ENABLE;
    }
}

/* ---- sanctum_exit --------------------------------------------------------
   Mark the sanctum session as over.  The caller is responsible for calling
   section_render_visible to restore the overworld BG0 view.
--------------------------------------------------------------------------- */
void sanctum_exit(Sanctum *s)
{
    in_sanctum = FALSE;

    /* Deactivate any lingering boss */
    active_boss.active = FALSE;

    /* If the boss was defeated, persist that flag on the Sanctum struct so
       it survives re-entry within the same play session. */
    if (boss_is_defeated())
        s->boss_defeated = TRUE;

    /* Caller must call section_render_visible to restore the overworld. */
}
