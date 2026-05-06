#include <tonc.h>
#include "tile_engine.h"
#include "demo_map.h"
#include "input.h"
#include "player.h"
#include "inventory.h"
#include "daytimer.h"
#include "hud.h"
#include "npc.h"
#include "inventory_screen.h"
#include "checkout.h"
#include "save.h"
#include "constants.h"

/*
 * Check whether the player is currently facing a register tile (COL_REGISTER).
 * Used by the main loop to decide whether A should trigger checkout.
 */
static u8 player_facing_register(void)
{
    s8 ix = (s8)g_player.x;
    s8 iy = (s8)g_player.y;
    u8 tile;

    switch (g_player.dir) {
    case DIR_DOWN:  iy++; break;
    case DIR_UP:    iy--; break;
    case DIR_LEFT:  ix--; break;
    case DIR_RIGHT: ix++; break;
    default: break;
    }

    if (ix < 0 || ix >= (s8)MAP_W || iy < 0 || iy >= (s8)MAP_H) return 0;

    tile = map_tile_at((u8)ix, (u8)iy);
    return (g_collision_flags[tile] & COL_REGISTER) ? 1u : 0u;
}

int main(void)
{
    u8 i;

    /* IRQ init — enable VBlank interrupt for VBlankIntrWait */
    irq_init(NULL);
    irq_add(II_VBLANK, NULL);

    /* Subsystem init (order matters — tile engine before map, map before player) */
    tile_engine_init();
    demo_map_load();     /* populates g_collision_flags[] */
    inventory_init();
    daytimer_init();
    hud_init();
    player_init();       /* must come after tile_engine_init (sets DCNT_OBJ flags) */
    npc_init();

    /* Clear input state before entering main loop */
    input_poll();

    /* Load save slot 0 if it exists */
    if (save_slot_exists(0)) {
        save_read(0);
    }

    /* ----------------------------------------------------------------
     * Main game loop
     * ---------------------------------------------------------------- */
    while (1) {
        /* Wait for VBlank — all OAM / VRAM writes happen here */
        VBlankIntrWait();

        /* Read input state for this frame */
        input_poll();

        if (g_daytimer.store_open) {

            /* UPDATE PHASE — no VRAM writes allowed here in practice,
             * but on GBA with simple screen writes this is safe during VBlank. */

            /* Advance patience minute tracking before NPC updates */
            g_npc_patience_minute = g_daytimer.minute;

            /* SELECT opens inventory screen (blocks until closed) */
            if (KEY_PRESSED(KEY_SELECT)) {
                inventory_screen_open();
                /* inventory_screen_open() is blocking; input re-polled inside */
                continue;  /* restart loop so we re-enter VBlankIntrWait cleanly */
            }

            /* A at register → checkout (takes priority over player_update) */
            if (KEY_PRESSED(KEY_A) && player_facing_register()) {
                checkout_start();
                continue;
            }

            /* Player movement and interact */
            player_update();

            /* Day timer advance */
            daytimer_update();

            /* NPC spawn check */
            if (g_npc_spawn_timer > 0) {
                g_npc_spawn_timer--;
            } else {
                /* Check if any NPC slot is free before spawning */
                u8 active_count = 0;
                for (i = 0; i < MAX_NPCS; i++) {
                    if (g_npcs[i].active) active_count++;
                }
                if (active_count == 0) {
                    npc_spawn();
                }
                g_npc_spawn_timer = NPC_SPAWN_INTERVAL;
            }

            /* Update all active NPCs */
            for (i = 0; i < MAX_NPCS; i++) {
                if (g_npcs[i].active) {
                    npc_update(&g_npcs[i]);
                }
            }
        }

        /* DRAW PHASE — runs every frame, store open or not.
         * Flush deferred VRAM writes first (still in VBlank window). */
        demo_map_flush_dirty();
        hud_draw();

        for (i = 0; i < MAX_NPCS; i++) {
            if (g_npcs[i].active) {
                npc_draw(&g_npcs[i]);
            }
        }

        player_draw();
    }

    return 0;
}
