#include "game.h"
#include "graphics.h"

/* -----------------------------------------------------------------------
 * Brick color table: row 0 (top) to row 5 (bottom)
 * ----------------------------------------------------------------------- */
const u16 BRICK_COLORS[BRICK_ROWS] = {
    RGB15(31,  0,  0),  /* row 0: red    */
    RGB15(31, 14,  0),  /* row 1: orange */
    RGB15(31, 28,  0),  /* row 2: yellow */
    RGB15( 0, 26,  0),  /* row 3: green  */
    RGB15( 0, 22, 26),  /* row 4: cyan   */
    RGB15( 8,  8, 31),  /* row 5: blue   */
};

/* ----------------------------------------------------------------------- */
static int aabb_overlap(int ax, int ay, int aw, int ah,
                        int bx, int by, int bw, int bh) {
    return (ax < bx + bw) && (ax + aw > bx) &&
           (ay < by + bh) && (ay + ah > by);
}

/* Boost ball speed by SPEED_BUMP (preserves sign, caps at MAX_SPEED) */
static void ball_boost(Ball *ball) {
    int ax = ABS(ball->vx) + SPEED_BUMP;
    int ay = ABS(ball->vy) + SPEED_BUMP;
    if (ax > MAX_SPEED) ax = MAX_SPEED;
    if (ay > MAX_SPEED) ay = MAX_SPEED;
    ball->vx = (ball->vx >= 0) ? ax : -ax;
    ball->vy = (ball->vy >= 0) ? ay : -ay;
}

/* Ensure |vx| is at least MIN_VX (prevents purely vertical ball) */
static void enforce_min_vx(Ball *ball) {
    if (ABS(ball->vx) < MIN_VX)
        ball->vx = (ball->vx >= 0) ? MIN_VX : -MIN_VX;
}

/* -----------------------------------------------------------------------
 * game_init: full reset to title state
 * ----------------------------------------------------------------------- */
void game_init(Game *g) {
    g->score        = 0;
    g->lives        = LIVES_START;
    g->level        = 1;
    g->state        = STATE_TITLE;
    g->frame_count  = 0;
    g->screen_dirty = 0;
    g->score_dirty  = 0;
    game_initLevel(g);
}

/* -----------------------------------------------------------------------
 * game_initLevel: reset ball, paddle, bricks for current level
 * ----------------------------------------------------------------------- */
void game_initLevel(Game *g) {
    int row, col, i;

    /* Paddle starts centered */
    g->paddle.x    = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
    g->prev_paddle_x = g->paddle.x;

    /* Ball starts stuck to paddle, center-aligned */
    {
        int bpx = g->paddle.x + PADDLE_WIDTH / 2 - BALL_SIZE / 2;
        int bpy = PADDLE_Y - BALL_SIZE - 1;
        g->ball.x        = bpx * FP_SCALE;
        g->ball.y        = bpy * FP_SCALE;
        g->prev_ball_px  = bpx;
        g->prev_ball_py  = bpy;
        g->ball.launched = 0;
    }

    /* Ball velocity: base + per-level bonus */
    {
        int bonus = (g->level - 1) * LEVEL_SPEED;
        g->ball.vx = BALL_BASE_VX + bonus;
        g->ball.vy = BALL_BASE_VY - bonus;  /* vy is negative (upward) */
    }

    /* Build brick grid */
    g->bricks_remaining = TOTAL_BRICKS;
    for (row = 0; row < BRICK_ROWS; row++) {
        for (col = 0; col < BRICK_COLS; col++) {
            i = row * BRICK_COLS + col;
            g->bricks[i].x     = BRICK_ORIGIN_X + col * (BRICK_WIDTH  + BRICK_GAP);
            g->bricks[i].y     = BRICK_ORIGIN_Y + row * (BRICK_HEIGHT + BRICK_GAP);
            g->bricks[i].color = BRICK_COLORS[row];
            g->bricks[i].alive = 1;
        }
    }

    g->score_dirty  = 1;
    g->screen_dirty = 1;
}

/* -----------------------------------------------------------------------
 * game_update: called once per frame during STATE_PLAYING
 * ----------------------------------------------------------------------- */
void game_update(Game *g, u16 keys) {
    int bpx, bpy, i;
    Ball *ball = &g->ball;

    /* --- Move paddle --- */
    if (keys & KEY_LEFT) {
        g->paddle.x -= PADDLE_SPEED;
        if (g->paddle.x < 0) g->paddle.x = 0;
    }
    if (keys & KEY_RIGHT) {
        g->paddle.x += PADDLE_SPEED;
        if (g->paddle.x > SCREEN_WIDTH - PADDLE_WIDTH)
            g->paddle.x = SCREEN_WIDTH - PADDLE_WIDTH;
    }

    /* --- Ball: track paddle if not launched --- */
    if (!ball->launched) {
        ball->x = (g->paddle.x + PADDLE_WIDTH / 2 - BALL_SIZE / 2) * FP_SCALE;
        if (keys & KEY_A) {
            ball->launched = 1;
            /* Alternate launch direction by level so it's not always right */
            if (g->level % 2 == 0) ball->vx = -ABS(ball->vx);
        }
        return;
    }

    /* --- Move ball --- */
    ball->x += ball->vx;
    ball->y += ball->vy;

    bpx = ball->x / FP_SCALE;
    bpy = ball->y / FP_SCALE;

    /* --- Wall collisions (resolve before brick/paddle) --- */

    /* Left wall */
    if (ball->x < 0) {
        ball->x  = 0;
        ball->vx = ABS(ball->vx);
        enforce_min_vx(ball);
        bpx = 0;
    }
    /* Right wall */
    if (ball->x > (SCREEN_WIDTH - BALL_SIZE) * FP_SCALE) {
        ball->x  = (SCREEN_WIDTH - BALL_SIZE) * FP_SCALE;
        ball->vx = -ABS(ball->vx);
        enforce_min_vx(ball);
        bpx = SCREEN_WIDTH - BALL_SIZE;
    }
    /* Top wall (keep ball below HUD separator) */
    if (ball->y < PLAY_TOP_Y * FP_SCALE) {
        ball->y  = PLAY_TOP_Y * FP_SCALE;
        ball->vy = ABS(ball->vy);
        bpy = PLAY_TOP_Y;
    }
    /* Bottom: lose a life */
    if (ball->y > SCREEN_HEIGHT * FP_SCALE) {
        g->lives--;
        g->score_dirty = 1;
        if (g->lives <= 0) {
            g->state        = STATE_GAMEOVER;
            g->screen_dirty = 1;
        } else {
            /* Re-stick ball to paddle */
            {
                int nx = g->paddle.x + PADDLE_WIDTH / 2 - BALL_SIZE / 2;
                int ny = PADDLE_Y - BALL_SIZE - 1;
                ball->x = nx * FP_SCALE;
                ball->y = ny * FP_SCALE;
            }
            ball->launched = 0;
            /* Reset velocity to level base */
            {
                int bonus = (g->level - 1) * LEVEL_SPEED;
                ball->vx = BALL_BASE_VX + bonus;
                ball->vy = BALL_BASE_VY - bonus;
            }
        }
        return;
    }

    /* --- Paddle collision (only if ball moving downward) --- */
    if (ball->vy > 0 &&
        aabb_overlap(bpx, bpy, BALL_SIZE, BALL_SIZE,
                     g->paddle.x, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT)) {
        int center_offset;

        /* Snap ball above paddle */
        ball->y  = (PADDLE_Y - BALL_SIZE) * FP_SCALE;
        ball->vy = -ABS(ball->vy);

        /* Adjust vx based on hit position: hitting edge curves the ball */
        center_offset = (bpx + BALL_SIZE / 2) - (g->paddle.x + PADDLE_WIDTH / 2);
        ball->vx += center_offset * 4;
        ball->vx  = CLAMP(ball->vx, -MAX_SPEED, MAX_SPEED);
        enforce_min_vx(ball);
        return;
    }

    /* --- Brick collision --- */
    for (i = 0; i < TOTAL_BRICKS; i++) {
        Brick *br = &g->bricks[i];
        int pen_l, pen_r, pen_t, pen_b, min_px, min_py;

        if (!br->alive) continue;

        if (!aabb_overlap(bpx, bpy, BALL_SIZE, BALL_SIZE,
                          br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT))
            continue;

        /* Erase brick */
        gba_fillRect(br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT, COLOR_PLAY_BG);
        br->alive = 0;
        g->bricks_remaining--;
        g->score      += SCORE_PER_BRICK;
        g->score_dirty = 1;

        /* Speed boost */
        ball_boost(ball);

        /* Penetration-depth axis selection */
        pen_l = (bpx + BALL_SIZE) - br->x;
        pen_r = (br->x + BRICK_WIDTH)  - bpx;
        pen_t = (bpy + BALL_SIZE) - br->y;
        pen_b = (br->y + BRICK_HEIGHT) - bpy;

        min_px = MIN(pen_l, pen_r);
        min_py = MIN(pen_t, pen_b);

        if (min_px < min_py) {
            /* Side hit: flip vx */
            ball->vx = -ball->vx;
        } else {
            /* Top/bottom hit: flip vy */
            ball->vy = -ball->vy;
        }

        /* Level cleared? */
        if (g->bricks_remaining == 0) {
            if (g->level >= MAX_LEVEL) {
                g->state        = STATE_WIN;
                g->screen_dirty = 1;
            } else {
                g->level++;
                game_initLevel(g);  /* screen_dirty set inside */
            }
        }

        break;  /* one brick per frame */
    }
}

/* -----------------------------------------------------------------------
 * game_drawHUD: score, level, lives bar
 * ----------------------------------------------------------------------- */
void game_drawHUD(const Game *g) {
    char buf[8];
    int  i;

    /* Background */
    gba_fillRect(0, 0, SCREEN_WIDTH, HUD_HEIGHT, COLOR_HUD_BG);
    /* Separator */
    gba_drawHLine(0, HUD_SEP_Y, SCREEN_WIDTH, COLOR_SEP);

    /* "SCORE:" label + value */
    gba_drawString(2, 2, "SCORE:", COLOR_HUD_FG, COLOR_HUD_BG);
    gba_drawNumber(38, 2, g->score, COLOR_YELLOW, COLOR_HUD_BG);

    /* "LVL:" + value (centered area) */
    gba_drawString(110, 2, "LVL:", COLOR_HUD_FG, COLOR_HUD_BG);
    gba_drawNumber(134, 2, g->level, COLOR_CYAN, COLOR_HUD_BG);

    /* Lives: colored squares on right */
    gba_drawString(160, 2, "LIVES:", COLOR_HUD_FG, COLOR_HUD_BG);
    for (i = 0; i < g->lives; i++)
        gba_fillRect(200 + i * 9, 2, 7, 7, COLOR_RED);
    /* Erase any leftover squares (lives may have decreased) */
    for (i = g->lives; i < LIVES_START; i++)
        gba_fillRect(200 + i * 9, 2, 7, 7, COLOR_HUD_BG);

    (void)buf;
}

/* -----------------------------------------------------------------------
 * game_drawBricks: draw all alive bricks (call on screen_dirty)
 * ----------------------------------------------------------------------- */
void game_drawBricks(const Game *g) {
    int i;
    for (i = 0; i < TOTAL_BRICKS; i++) {
        const Brick *br = &g->bricks[i];
        if (br->alive)
            gba_fillRect(br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT, br->color);
    }
}

/* -----------------------------------------------------------------------
 * game_draw: dirty-rectangle per-frame draw (STATE_PLAYING)
 * ----------------------------------------------------------------------- */
void game_draw(Game *g) {
    int bpx, bpy, i;

    /* Full redraw on level start / state transition */
    if (g->screen_dirty) {
        gba_clearScreen(COLOR_PLAY_BG);
        game_drawBricks(g);
        gba_fillRect(g->paddle.x, PADDLE_Y, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_PADDLE);
        {
            int px = g->ball.x / FP_SCALE;
            int py = g->ball.y / FP_SCALE;
            gba_fillRect(px, py, BALL_SIZE, BALL_SIZE, COLOR_BALL);
            g->prev_ball_px = px;
            g->prev_ball_py = py;
        }
        game_drawHUD(g);
        g->screen_dirty = 0;
        g->score_dirty  = 0;
        return;
    }

    /* --- Erase old ball --- */
    gba_fillRect(g->prev_ball_px, g->prev_ball_py,
                 BALL_SIZE, BALL_SIZE, COLOR_PLAY_BG);

    /* Restore any alive bricks that were under the old ball position */
    for (i = 0; i < TOTAL_BRICKS; i++) {
        const Brick *br = &g->bricks[i];
        if (!br->alive) continue;
        if (aabb_overlap(g->prev_ball_px, g->prev_ball_py, BALL_SIZE, BALL_SIZE,
                         br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT))
            gba_fillRect(br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT, br->color);
    }

    /* --- Erase old paddle --- */
    gba_fillRect(g->prev_paddle_x, PADDLE_Y,
                 PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_PLAY_BG);

    /* --- Draw new paddle --- */
    gba_fillRect(g->paddle.x, PADDLE_Y,
                 PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_PADDLE);
    g->prev_paddle_x = g->paddle.x;

    /* --- Draw new ball --- */
    bpx = g->ball.x / FP_SCALE;
    bpy = g->ball.y / FP_SCALE;
    gba_fillRect(bpx, bpy, BALL_SIZE, BALL_SIZE, COLOR_BALL);
    g->prev_ball_px = bpx;
    g->prev_ball_py = bpy;

    /* --- HUD update --- */
    if (g->score_dirty) {
        game_drawHUD(g);
        g->score_dirty = 0;
    }
}

/* -----------------------------------------------------------------------
 * Title screen
 * ----------------------------------------------------------------------- */
void draw_titleScreen(Game *g) {
    int i, x, y;
    const char *title = "BRICK BREAKER GBA";
    const char *sub   = "A GBA BREAKOUT CLONE";
    const char *inst1 = "D-PAD: MOVE PADDLE";
    const char *inst2 = "A: LAUNCH BALL";
    const char *inst3 = "START: PAUSE";

    gba_clearScreen(COLOR_BLACK);

    /* Title with shadow */
    x = (SCREEN_WIDTH - gba_stringWidth(title)) / 2;
    y = 22;
    gba_drawString(x + 1, y + 1, title, COLOR_SHADOW, COLOR_BLACK);
    gba_drawString(x,     y,     title, COLOR_TITLE,  COLOR_BLACK);

    /* Subtitle */
    x = (SCREEN_WIDTH - gba_stringWidth(sub)) / 2;
    gba_drawString(x, 38, sub, COLOR_GRAY, COLOR_BLACK);

    /* Decorative mini brick grid (6 rows × 7px + 1px gap = 8px each) */
    for (i = 0; i < BRICK_ROWS; i++) {
        int bx;
        for (bx = 0; bx < 8; bx++) {
            gba_fillRect(4 + bx * 29, 50 + i * 8,
                         28, 7, BRICK_COLORS[i]);
        }
    }

    /* Instructions */
    x = (SCREEN_WIDTH - gba_stringWidth(inst1)) / 2;
    gba_drawString(x, 124, inst1, COLOR_GRAY, COLOR_BLACK);
    x = (SCREEN_WIDTH - gba_stringWidth(inst2)) / 2;
    gba_drawString(x, 134, inst2, COLOR_GRAY, COLOR_BLACK);
    x = (SCREEN_WIDTH - gba_stringWidth(inst3)) / 2;
    gba_drawString(x, 144, inst3, COLOR_GRAY, COLOR_BLACK);

    /* "PRESS START" drawn by the flash loop in main (at y=110) */
    g->frame_count = 0;
    (void)y;
}

/* Update the flashing "PRESS START" text on title/win screen */
static void flash_press_start(u32 frame_count, int y, u16 bg) {
    const char *msg = "PRESS START";
    int x = (SCREEN_WIDTH - gba_stringWidth(msg)) / 2;
    u16 color = ((frame_count % 60) < 30) ? COLOR_WHITE : bg;
    gba_drawString(x, y, msg, color, bg);
}

/* -----------------------------------------------------------------------
 * Pause overlay
 * ----------------------------------------------------------------------- */
void draw_pauseOverlay(void) {
    int bx = 60, by = 60, bw = 120, bh = 40;
    gba_fillRect(bx,     by,     bw,     bh,     COLOR_DARKGRAY);
    gba_drawRect(bx,     by,     bw,     bh,     COLOR_WHITE);
    {
        const char *p = "PAUSED";
        int x = bx + (bw - gba_stringWidth(p)) / 2;
        gba_drawString(x, by + 8,  p,           COLOR_WHITE, COLOR_DARKGRAY);
    }
    {
        const char *p = "START TO RESUME";
        int x = bx + (bw - gba_stringWidth(p)) / 2;
        gba_drawString(x, by + 24, p,           COLOR_CYAN,  COLOR_DARKGRAY);
    }
}

/* -----------------------------------------------------------------------
 * Game over screen
 * ----------------------------------------------------------------------- */
void draw_gameOverScreen(const Game *g) {
    const char *go    = "GAME OVER";
    const char *sc    = "SCORE:";
    const char *retry = "START  TO RETRY";
    const char *back  = "SELECT TO TITLE";
    int x;

    gba_clearScreen(COLOR_BLACK);

    x = (SCREEN_WIDTH - gba_stringWidth(go)) / 2;
    gba_drawString(x + 1, 45 + 1, go, COLOR_SHADOW, COLOR_BLACK);
    gba_drawString(x,     45,     go, COLOR_RED,    COLOR_BLACK);

    x = (SCREEN_WIDTH - gba_stringWidth(sc)) / 2;
    gba_drawString(x, 70, sc, COLOR_WHITE, COLOR_BLACK);
    gba_drawNumber((SCREEN_WIDTH + gba_stringWidth(sc)) / 2 + 6,
                   70, g->score, COLOR_YELLOW, COLOR_BLACK);

    x = (SCREEN_WIDTH - gba_stringWidth(retry)) / 2;
    gba_drawString(x, 100, retry, COLOR_GRAY, COLOR_BLACK);
    x = (SCREEN_WIDTH - gba_stringWidth(back)) / 2;
    gba_drawString(x, 112, back,  COLOR_GRAY, COLOR_BLACK);
}

/* -----------------------------------------------------------------------
 * Win screen
 * ----------------------------------------------------------------------- */
void draw_winScreen(const Game *g) {
    const char *win = "YOU WIN!";
    const char *sc  = "FINAL SCORE:";
    int i, x;

    gba_clearScreen(RGB15(0, 0, 4));

    /* Rainbow brick row */
    for (i = 0; i < BRICK_ROWS; i++)
        gba_fillRect(4 + i * 39, 28, 36, 10, BRICK_COLORS[i]);

    x = (SCREEN_WIDTH - gba_stringWidth(win)) / 2;
    gba_drawString(x + 1, 54 + 1, win, COLOR_SHADOW, RGB15(0, 0, 4));
    gba_drawString(x,     54,     win, COLOR_YELLOW, RGB15(0, 0, 4));

    x = (SCREEN_WIDTH - gba_stringWidth(sc)) / 2;
    gba_drawString(x, 76, sc, COLOR_WHITE, RGB15(0, 0, 4));
    gba_drawNumber((SCREEN_WIDTH + gba_stringWidth(sc)) / 2 + 6,
                   76, g->score, COLOR_YELLOW, RGB15(0, 0, 4));

    /* "PRESS START" drawn by flash loop in main */
    (void)i; (void)x;
}

/* ----------------------------------------------------------------------- *
 * Expose the flash helper for use by main.c                               *
 * ----------------------------------------------------------------------- */
void game_flashPressStart(u32 frame_count, int y, u16 bg) {
    flash_press_start(frame_count, y, bg);
}
