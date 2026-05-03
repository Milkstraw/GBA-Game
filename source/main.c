#include "gba.h"
#include "graphics.h"
#include "game.h"
#include "sound.h"

/* SRAM save-type identifier — emulators and flash carts detect this string */
const char SAVE_TYPE[] = "SRAM_V113";

int main(void) {
    Game game;
    u16  keys      = 0;
    u16  prev_keys = 0;
    u16  keys_new;

    /* Enable Mode 3 bitmap display with BG2 */
    REG_DISPCNT = MODE3 | BG2_ENABLE;

    /* BG2 affine matrix: identity (1.0 = 0x100 in 8.8 fixed-point) */
    REG_BG2PA = 0x0100;
    REG_BG2PB = 0;
    REG_BG2PC = 0;
    REG_BG2PD = 0x0100;
    REG_BG2X  = 0;
    REG_BG2Y  = 0;

    sound_init();
    game_init(&game);
    draw_titleScreen(&game);

    while (1) {
        vsync();
        update_shake(&game);

        prev_keys = keys;
        keys      = KEYS_READ();
        keys_new  = (u16)(keys & ~prev_keys);
        game.frame_count++;

        switch (game.state) {

        /* ---- Title screen ---- */
        case STATE_TITLE:
            game_flashPressStart(game.frame_count, 110, COLOR_BLACK);
            if (keys_new & KEY_START) {
                game.score = 0;
                game.lives = LIVES_START;
                game.level = 1;
                game_initLevel(&game);
                game.state = STATE_PLAYING;
                game_draw(&game);
            }
            break;

        /* ---- Main gameplay ---- */
        case STATE_PLAYING:
            if (keys_new & KEY_START) {
                game.state = STATE_PAUSED;
                draw_pauseOverlay();
            } else {
                game_update(&game, keys, keys_new);

                if (game.state == STATE_PLAYING) {
                    game_draw(&game);
                } else if (game.state == STATE_GAMEOVER) {
                    draw_gameOverScreen(&game);
                } else if (game.state == STATE_WIN) {
                    draw_winScreen(&game);
                }
            }
            break;

        /* ---- Paused ---- */
        case STATE_PAUSED:
            if (keys_new & KEY_START) {
                game.screen_dirty = 1;
                game_draw(&game);
                game.state = STATE_PLAYING;
            }
            break;

        /* ---- Game over ---- */
        case STATE_GAMEOVER:
            if (keys_new & KEY_START) {
                game.score = 0;
                game.lives = LIVES_START;
                game.level = 1;
                game_initLevel(&game);
                game.state = STATE_PLAYING;
                game_draw(&game);
            } else if (keys_new & KEY_SELECT) {
                game_init(&game);
                draw_titleScreen(&game);
            }
            break;

        /* ---- Win ---- */
        case STATE_WIN:
            game_flashPressStart(game.frame_count, 100, RGB15(0, 0, 4));
            if (keys_new & KEY_START) {
                game_init(&game);
                draw_titleScreen(&game);
            }
            break;
        }
    }

    return 0;
}
