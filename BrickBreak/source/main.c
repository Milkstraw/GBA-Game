#include "gba.h"
#include "graphics.h"
#include "game.h"

int main(void) {
    Game game;
    u16  keys      = 0;
    u16  prev_keys = 0;
    u16  keys_new;

    /* Enable Mode 3 bitmap display with BG2 */
    REG_DISPCNT = MODE3 | BG2_ENABLE;

    game_init(&game);
    draw_titleScreen(&game);

    while (1) {
        vsync();

        prev_keys = keys;
        keys      = KEYS_READ();
        keys_new  = (u16)(keys & ~prev_keys);

        switch (game.state) {

        /* ---- Title screen ---- */
        case STATE_TITLE:
            game.frame_count++;
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
                game_update(&game, keys);

                if (game.state == STATE_PLAYING) {
                    game_draw(&game);
                } else if (game.state == STATE_GAMEOVER) {
                    draw_gameOverScreen(&game);
                } else if (game.state == STATE_WIN) {
                    game.frame_count = 0;
                    draw_winScreen(&game);
                }
                /* STATE_PLAYING with screen_dirty (new level) handled by game_draw */
            }
            break;

        /* ---- Paused ---- */
        case STATE_PAUSED:
            if (keys_new & KEY_START) {
                /* Erase the pause overlay box */
                {
                    int bx = 60, by = 60, bw = 120, bh = 40, i;
                    gba_fillRect(bx, by, bw, bh, COLOR_PLAY_BG);
                    /* Restore any bricks that were under the box */
                    for (i = 0; i < TOTAL_BRICKS; i++) {
                        const Brick *br = &game.bricks[i];
                        if (!br->alive) continue;
                        if (br->x < bx + bw && br->x + BRICK_WIDTH  > bx &&
                            br->y < by + bh && br->y + BRICK_HEIGHT > by)
                            gba_fillRect(br->x, br->y,
                                         BRICK_WIDTH, BRICK_HEIGHT, br->color);
                    }
                }
                /* Redraw paddle and ball */
                gba_fillRect(game.paddle.x, PADDLE_Y,
                             PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_PADDLE);
                {
                    int bpx = game.ball.x / FP_SCALE;
                    int bpy = game.ball.y / FP_SCALE;
                    gba_fillRect(bpx, bpy, BALL_SIZE, BALL_SIZE, COLOR_BALL);
                }
                game_drawHUD(&game);
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
            game.frame_count++;
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
