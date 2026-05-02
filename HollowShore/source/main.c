/*
 * HollowShore — main.c
 * Wave 8: Full game loop with state machine.
 */

#include <stddef.h>
#include "gba.h"
#include "core/input.h"
#include "core/clock.h"
#include "core/save.h"
#include "core/endgame.h"
#include "core/debug.h"
#include "graphics/palette.h"
#include "graphics/sprites.h"
#include "graphics/renderer.h"
#include "world/tile.h"
#include "world/section.h"
#include "world/transition.h"
#include "world/maps/world_a.h"
#include "player/player.h"
#include "player/stats.h"
#include "player/inventory.h"
#include "systems/combat.h"
#include "systems/monsters.h"
#include "systems/farming.h"
#include "systems/crafting.h"
#include "systems/building.h"
#include "systems/weather.h"
#include "ui/text.h"
#include "ui/hud.h"
#include "ui/menus.h"
#include "sanctums/sanctum.h"
#include "sanctums/bosses.h"
#include "sanctums/roothold.h"
#include "sanctums/tidecrypt.h"
#include "sanctums/frostspire.h"
#include "sanctums/marshveil.h"
#include "sanctums/emberdepth.h"
#include "sanctums/hollow_throne.h"

/* ---- Game state enum ---------------------------------------------------- */

typedef enum {
    STATE_TITLE = 0,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_INVENTORY,
    STATE_CRAFTING,
    STATE_SANCTUM,
    STATE_ENDGAME
} GameState;

/* ---- Static globals (avoid large structs on the limited stack) ----------- */

static Player    player;
static Inventory inv;
static GameClock game_clock;
static Section   cur_section  __attribute__((section(".sbss")));
static GameState state       = STATE_TITLE;
static SaveData  save_buf;
static u16       night_count  = 0;
static u32       total_frames = 0;

/* Active sanctum for dungeon mode */
static Sanctum   cur_sanctum  __attribute__((section(".sbss")));

/* Crafting recipe cursor */
static u8 crafting_cursor = 0;

/* ---- Camera helpers ----------------------------------------------------- */

#define SCREEN_W  240
#define SCREEN_H  160
#define CAM_PIVOT_X  120
#define CAM_PIVOT_Y   80

#define SECTION_PX_W  (SECTION_TILES_W * TILE_SIZE)
#define SECTION_PX_H  (SECTION_TILES_H * TILE_SIZE)

static void camera_follow_player(void) {
    s16 cam_x = player.x - CAM_PIVOT_X;
    s16 cam_y = player.y - CAM_PIVOT_Y;

    /* Clamp horizontal */
    if (cam_x < 0) cam_x = 0;
    if (cam_x > SECTION_PX_W - SCREEN_W) cam_x = SECTION_PX_W - SCREEN_W;

    /* Clamp vertical */
    if (cam_y < 0) cam_y = 0;
    if (cam_y > SECTION_PX_H - SCREEN_H) cam_y = SECTION_PX_H - SCREEN_H;

    cur_section.cam_x = cam_x;
    cur_section.cam_y = cam_y;
}

/* ---- Derive transition direction from player position ------------------- */

static u8 direction_from_player_edge(void) {
    if (player.y <= 0)                       return DIR_UP;
    if (player.y >= SECTION_PX_H - TILE_SIZE) return DIR_DOWN;
    if (player.x <= 0)                       return DIR_LEFT;
    return DIR_RIGHT;
}

/* ---- Populate SaveData from current game state -------------------------- */

static void save_populate(void) {
    save_buf.section_id = cur_section.id;
    save_buf.player_x   = player.x;
    save_buf.player_y   = player.y;
    save_buf.day        = game_clock.day;
    save_buf.hour       = game_clock.hour;
    save_buf.season     = (u8)season_from_day(game_clock.day);
    save_buf.weather    = (u8)current_weather;
    /* boss_flags updated externally via sanctum system — preserve existing */
}

/* ---- State: Title screen ------------------------------------------------ */

static void update_title(void) {
    text_clear();
    text_draw(5, 8,  "HOLLOW SHORE");
    text_draw(4, 11, "PRESS START");

    if (key_pressed(KEY_START)) {
        text_clear();
        state = STATE_PLAYING;
    }
}

/* ---- State: Playing ----------------------------------------------------- */

static void update_playing(void) {
    u16 keys = key_held(0x03FFu);

    /* Update player */
    player_update(&player, keys);

    /* Camera */
    camera_follow_player();

    /* Render world */
    section_render_visible(&cur_section, cur_section.cam_x, cur_section.cam_y);

    /* Draw player */
    player_draw(&player, cur_section.cam_x, cur_section.cam_y);

    /* Update enemies */
    enemy_update_all(
        player.x, player.y,
        game_clock.day,
        clock_get_phase(&game_clock) == PHASE_NIGHT
    );

    /* Update projectiles */
    projectile_update();

    /* HUD */
    hud_draw(&player.stats, &inv);

    /* Clock tick */
    clock_update(&game_clock);

    /* New-day events — fires at midnight (hour wraps to 0) once per day */
    if (game_clock.hour == 0 &&
        game_clock.frame_counter > 0 &&
        (game_clock.frame_counter % (60u * 60u * 24u)) == 0u) {
        farming_tick(game_clock.day);
        weather_update(game_clock.day, season_from_day(game_clock.day));
        night_count++;
        siege_event_check(game_clock.day, night_count);
    }

    /* Section transitions */
    transition_check(player.x, player.y, cur_section.id);
    transition_update();

    if (transition_get_state() == TRANS_LOADING) {
        u8 dir    = direction_from_player_edge();
        u8 new_id = section_id_from_direction(cur_section.id, dir);
        section_save_state(&cur_section);
        section_load(new_id, &cur_section);
    }

    /* Boss defeated while in overworld combat? */
    if (boss_is_defeated()) {
        cur_sanctum.boss_defeated = TRUE;
        sanctum_exit(NULL);
        state = STATE_PLAYING;
    }

    /* Game over */
    if (player.stats.hp <= 0) {
        text_clear();
        text_draw(6, 9, "GAME OVER");
        state = STATE_TITLE;
    }

    /* State transitions via buttons */
    if (key_pressed(KEY_START))  state = STATE_PAUSED;
    if (key_pressed(KEY_SELECT)) state = STATE_INVENTORY;
}

/* ---- State: Paused ------------------------------------------------------ */

static void update_paused(void) {
    draw_pause_menu();

    if (key_pressed(KEY_START)) {
        state = STATE_PLAYING;
    }

    if (key_pressed(KEY_B)) {
        state = STATE_PLAYING;
    }

    /* SELECT saves the game while paused */
    if (key_pressed(KEY_SELECT)) {
        save_populate();
        save_write(&save_buf);
    }
}

/* ---- State: Inventory --------------------------------------------------- */

static void update_inventory(void) {
    draw_inventory_screen(&inv);

    /* Cycle hotbar cursor with L/R */
    if (key_pressed(KEY_R)) {
        if (inv.hotbar_cursor < HOTBAR_SLOTS - 1u)
            inv.hotbar_cursor++;
        else
            inv.hotbar_cursor = 0;
    }
    if (key_pressed(KEY_L)) {
        if (inv.hotbar_cursor > 0)
            inv.hotbar_cursor--;
        else
            inv.hotbar_cursor = HOTBAR_SLOTS - 1u;
    }

    if (key_pressed(KEY_SELECT) || key_pressed(KEY_B)) {
        state = STATE_PLAYING;
    }
}

/* ---- State: Crafting ---------------------------------------------------- */

static void update_crafting(void) {
    draw_crafting_screen(&inv);

    /* Navigate recipe list */
    if (key_pressed(KEY_DOWN)) {
        if (crafting_cursor < recipe_count - 1u)
            crafting_cursor++;
    }
    if (key_pressed(KEY_UP)) {
        if (crafting_cursor > 0)
            crafting_cursor--;
    }

    /* Attempt to craft selected recipe */
    if (key_pressed(KEY_A)) {
        crafting_execute(crafting_cursor, &inv);
    }

    if (key_pressed(KEY_B)) {
        state = STATE_PLAYING;
    }
}

/* ---- State: Sanctum ----------------------------------------------------- */

static void update_sanctum(void) {
    u16 keys = key_held(0x03FFu);

    /* Movement inside dungeon (no camera offset — fits 240×160) */
    player_update(&player, keys);
    player_draw(&player, 0, 0);

    /* Dungeon enemies */
    enemy_update_all(
        player.x, player.y,
        game_clock.day,
        clock_get_phase(&game_clock) == PHASE_NIGHT
    );

    /* Boss update — dispatched by type */
    if (active_boss.active) {
        switch (active_boss.type) {
            case BOSS_ROOT_WARDEN:
                root_warden_update(&active_boss, player.x, player.y);
                break;
            case BOSS_DROWNED_WARDEN:
                drowned_warden_update(&active_boss, player.x, player.y);
                break;
            case BOSS_GLACIAL_WARDEN:
                glacial_warden_update(&active_boss, player.x, player.y);
                break;
            case BOSS_BOG_SPECTER:
                bog_specter_update(&active_boss, player.x, player.y);
                break;
            case BOSS_EMBER_TITAN:
                ember_titan_update(&active_boss, player.x, player.y);
                break;
            case BOSS_HOLLOW_SOVEREIGN:
                hollow_sovereign_update(&active_boss, player.x, player.y, &keys);
                break;
        }
    }

    hud_draw(&player.stats, &inv);

    /* Boss defeated: exit sanctum */
    if (boss_is_defeated()) {
        cur_sanctum.boss_defeated = TRUE;
        sanctum_exit(NULL);
        state = STATE_PLAYING;
    }

    /* Manual exit */
    if (key_pressed(KEY_B)) {
        sanctum_exit(NULL);
        state = STATE_PLAYING;
    }
}

/* ---- State: Endgame ----------------------------------------------------- */

static void update_endgame(void) {
    credits_roll("Hollow Shore", total_frames / 60u);
    /* After credits, return to title */
    state = STATE_TITLE;
}

/* ---- Entry point -------------------------------------------------------- */

int main(void) {
    /* 1. Debug init must come first */
    debug_init();
    debug_log(LOG_INFO, "HollowShore: startup");

    /* 2. Hardware display config */
    REG_DISPCNT = MODE0 | BG0_ENABLE | BG3_ENABLE | OBJ_ENABLE;

    /* 3. Subsystem init */
    oam_init();
    text_init();
    init_renderer();
    key_poll();         /* prime the key state */
    crafting_init();
    enemy_init_pool();

    /* 4. Default player / clock / inventory */
    player_init(&player);
    inventory_init(&inv);
    clock_init(&game_clock);

    /* 5. Attempt to restore save data */
    save_read(&save_buf);
    if (save_validate_checksum(&save_buf)) {
        /* Restore persisted fields */
        player.x         = save_buf.player_x;
        player.y         = save_buf.player_y;
        game_clock.day   = save_buf.day;
        game_clock.hour  = save_buf.hour;
        section_load(save_buf.section_id, &cur_section);
        debug_log(LOG_INFO, "save restored");
    } else {
        /* Fresh game: start in section 5 (centre of 3×3 grid) */
        section_load(5, &cur_section);
        debug_log(LOG_INFO, "no save — fresh game");
    }

    /* 6. Initial render */
    camera_follow_player();
    section_render_visible(&cur_section, cur_section.cam_x, cur_section.cam_y);

    /* ---- Main loop ------------------------------------------------------- */
    while (1) {
        vsync();
        key_poll();
        total_frames++;

        switch (state) {
            case STATE_TITLE:     update_title();     break;
            case STATE_PLAYING:   update_playing();   break;
            case STATE_PAUSED:    update_paused();    break;
            case STATE_INVENTORY: update_inventory(); break;
            case STATE_CRAFTING:  update_crafting();  break;
            case STATE_SANCTUM:   update_sanctum();   break;
            case STATE_ENDGAME:   update_endgame();   break;
        }

        copy_oam();
    }

    return 0;
}
