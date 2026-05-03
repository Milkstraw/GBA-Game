#include "game.h"
#include "graphics.h"
#include "sound.h"
#include "levels.h"

/* -----------------------------------------------------------------------
 * SRAM helpers (byte-by-byte — GBA SRAM has an 8-bit bus)
 * ----------------------------------------------------------------------- */
#define SRAM_MAGIC 0x42

static int sram_load_high_score(void) {
    if (SRAM_BASE[0] != SRAM_MAGIC) return 0;
    return ((int)SRAM_BASE[1] << 16) |
           ((int)SRAM_BASE[2] <<  8) |
            (int)SRAM_BASE[3];
}

static void sram_save_high_score(int score) {
    SRAM_BASE[0] = SRAM_MAGIC;
    SRAM_BASE[1] = (u8)((score >> 16) & 0xFF);
    SRAM_BASE[2] = (u8)((score >>  8) & 0xFF);
    SRAM_BASE[3] = (u8)( score        & 0xFF);
}

/* -----------------------------------------------------------------------
 * Brick helpers
 * ----------------------------------------------------------------------- */
static int brick_is_alive(const Brick *br) {
    return (br->max_hits == 0) || (br->hits_left > 0);
}

static u16 brick_display_color(const Brick *br) {
    int rr, gg, bb, pct;
    if (br->max_hits == 0) return COLOR_STEEL;
    if (br->max_hits == 1 || br->hits_left >= br->max_hits)
        return br->full_color;
    /* Scale brightness: 100% at full health, 40% at 1 hit left */
    pct = 40 + 60 * (br->hits_left - 1) / (br->max_hits - 1);
    rr = ((br->full_color & 0x1F)        * pct / 100);
    gg = (((br->full_color >>  5) & 0x1F) * pct / 100);
    bb = (((br->full_color >> 10) & 0x1F) * pct / 100);
    return RGB15(rr, gg, bb);
}

/* -----------------------------------------------------------------------
 * Ball physics helpers
 * ----------------------------------------------------------------------- */
static void ball_boost(Ball *ball) {
    int ax = ABS(ball->vx) + SPEED_BUMP;
    int ay = ABS(ball->vy) + SPEED_BUMP;
    if (ax > MAX_SPEED) ax = MAX_SPEED;
    if (ay > MAX_SPEED) ay = MAX_SPEED;
    ball->vx = (ball->vx >= 0) ?  ax : -ax;
    ball->vy = (ball->vy >= 0) ?  ay : -ay;
}

static void enforce_min_vx(Ball *ball) {
    if (ABS(ball->vx) < MIN_VX)
        ball->vx = (ball->vx >= 0) ? MIN_VX : -MIN_VX;
}

/* -----------------------------------------------------------------------
 * AABB overlap
 * ----------------------------------------------------------------------- */
static int aabb(int ax, int ay, int aw, int ah,
                int bx, int by, int bw, int bh) {
    return (ax < bx + bw) && (ax + aw > bx) &&
           (ay < by + bh) && (ay + ah > by);
}

/* -----------------------------------------------------------------------
 * Dirty-rect helper: restore any alive bricks in the given region
 * ----------------------------------------------------------------------- */
static void restore_bricks(const Game *g, int x, int y, int w, int h) {
    int i;
    for (i = 0; i < TOTAL_BRICKS; i++) {
        const Brick *br = &g->bricks[i];
        if (!brick_is_alive(br)) continue;
        if (aabb(x, y, w, h, br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT))
            gba_fillRect(br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT,
                         brick_display_color(br));
    }
}

/* -----------------------------------------------------------------------
 * Paddle color reflects active effects
 * ----------------------------------------------------------------------- */
static u16 get_paddle_color(const Effects *e) {
    if (e->laser_timer  > 0) return RGB15(28, 4,  4);   /* red   */
    if (e->sticky_timer > 0) return RGB15(31, 28, 0);   /* yellow*/
    if (e->wide_timer   > 0) return RGB15( 4,  4, 28);  /* blue  */
    return COLOR_PADDLE;
}

/* -----------------------------------------------------------------------
 * Power-up drawing
 * ----------------------------------------------------------------------- */
static const u16 PU_COLORS[POWERUP_COUNT] = {
    0,
    RGB15( 4,  4, 28),   /* WIDE      */
    RGB15(31, 28,  0),   /* MULTIBALL */
    RGB15( 0, 20,  0),   /* SLOW      */
    RGB15(28,  4,  4),   /* LASER     */
    RGB15( 0, 22, 22),   /* STICKY    */
    RGB15(28,  0, 22),   /* LIFE      */
};
static const char PU_LETTERS[] = " WMSLP+";

static void draw_powerup(const PowerUp *pu) {
    u16 c = PU_COLORS[pu->type];
    gba_fillRect(pu->x, pu->y, POWERUP_W, POWERUP_H, c);
    gba_drawRect(pu->x, pu->y, POWERUP_W, POWERUP_H, COLOR_WHITE);
    gba_drawChar(pu->x + 5, pu->y + 1,
                 PU_LETTERS[pu->type], COLOR_WHITE, c);
}

/* -----------------------------------------------------------------------
 * Random helpers
 * ----------------------------------------------------------------------- */
static u32 rand_next(Game *g) {
    g->rand_seed = g->rand_seed * 1664525u + 1013904223u;
    return g->rand_seed;
}

/* -----------------------------------------------------------------------
 * Reset balls[0] to paddle (after life lost); deactivate extras
 * ----------------------------------------------------------------------- */
static void game_reset_balls(Game *g) {
    int i, bpx, bpy;
    bpx = g->paddle.x + g->effects.paddle_width / 2 - BALL_SIZE / 2;
    bpy = PADDLE_Y - BALL_SIZE - 1;

    g->balls[0].x        = bpx * FP_SCALE;
    g->balls[0].y        = bpy * FP_SCALE;
    g->balls[0].launched = 0;
    g->balls[0].active   = 1;
    g->balls[0].prev_px  = -1;

    for (i = 1; i < MAX_BALLS; i++) {
        g->balls[i].active  = 0;
        g->balls[i].prev_px = -1;
    }
    g->ball_count = 1;
}

/* -----------------------------------------------------------------------
 * Apply a caught power-up effect
 * ----------------------------------------------------------------------- */
static void apply_multiball(Game *g) {
    int i;
    for (i = 1; i < MAX_BALLS; i++) {
        if (g->balls[i].active) continue;
        g->balls[i]          = g->balls[0];
        g->balls[i].active   = 1;
        g->balls[i].launched = 1;
        g->balls[i].prev_px  = -1;
        /* Diverge angle slightly */
        g->balls[i].vx      += (i == 1) ? -80 : 80;
        if (g->balls[i].vy > 0) g->balls[i].vy = -g->balls[i].vy;
        g->balls[i].vx = CLAMP(g->balls[i].vx, -MAX_SPEED, MAX_SPEED);
        enforce_min_vx(&g->balls[i]);
        g->ball_count++;
    }
}

static void game_apply_powerup(Game *g, int type) {
    sound_powerup_catch();
    switch ((PowerUpType)type) {
    case POWERUP_WIDE:
        g->effects.wide_timer   = EFFECT_WIDE_DURATION;
        g->effects.paddle_width = PADDLE_WIDTH_WIDE;
        break;
    case POWERUP_MULTIBALL:
        apply_multiball(g);
        break;
    case POWERUP_SLOW:
        g->effects.slow_timer = EFFECT_SLOW_DURATION;
        break;
    case POWERUP_LASER:
        g->effects.laser_timer = EFFECT_LASER_DURATION;
        break;
    case POWERUP_STICKY:
        g->effects.sticky_timer = EFFECT_STICKY_DURATION;
        break;
    case POWERUP_LIFE:
        if (g->lives < MAX_LIVES) { g->lives++; g->score_dirty = 1; }
        break;
    default:
        break;
    }
}

/* -----------------------------------------------------------------------
 * Try to drop a power-up from a destroyed brick (~25 % chance)
 * ----------------------------------------------------------------------- */
static void try_drop_powerup(Game *g, const Brick *br) {
    int i;
    PowerUp *slot = NULL;
    if ((rand_next(g) >> 24) >= 64) return;          /* 64/256 ≈ 25 % */
    for (i = 0; i < MAX_POWERUPS; i++)
        if (!g->powerups[i].active) { slot = &g->powerups[i]; break; }
    if (!slot) return;
    slot->type   = (int)(1 + (rand_next(g) >> 20) % (POWERUP_COUNT - 1));
    slot->x      = br->x + (BRICK_WIDTH - POWERUP_W) / 2;
    slot->y      = br->y;
    slot->y_fp   = br->y * FP_SCALE;
    slot->prev_y = -1;
    slot->active = 1;
}

/* -----------------------------------------------------------------------
 * Update effect timers each frame
 * ----------------------------------------------------------------------- */
static void update_effects(Game *g) {
    Effects *e = &g->effects;
    int i;

    if (e->wide_timer > 0) {
        if (--e->wide_timer == 0)
            e->paddle_width = PADDLE_WIDTH_NORMAL;
    }
    if (e->slow_timer   > 0) --e->slow_timer;
    if (e->sticky_timer > 0) --e->sticky_timer;
    if (e->laser_timer  > 0) {
        if (--e->laser_timer == 0) {
            for (i = 0; i < MAX_BULLETS; i++) {
                if (g->bullets[i].active) {
                    g->bullets[i].active = 0;
                    g->bullets[i].prev_y = -1;
                }
            }
        }
    }
    if (e->laser_cooldown > 0) --e->laser_cooldown;
}

/* -----------------------------------------------------------------------
 * Screen shake (BG2 scroll registers)
 * ----------------------------------------------------------------------- */
void update_shake(Game *g) {
    if (g->shake_timer > 0) {
        int ox, oy;
        --g->shake_timer;
        ox = (int)((rand_next(g) >> 20) & 7) - 3;
        oy = (int)((rand_next(g) >> 24) & 3) - 1;
        REG_BG2X = (s32)(ox << 8);
        REG_BG2Y = (s32)(oy << 8);
    } else {
        REG_BG2X = 0;
        REG_BG2Y = 0;
    }
}

/* -----------------------------------------------------------------------
 * game_init: full reset; load high score from SRAM
 * ----------------------------------------------------------------------- */
void game_init(Game *g) {
    int i;
    g->score        = 0;
    g->high_score   = sram_load_high_score();
    g->lives        = LIVES_START;
    g->level        = 1;
    g->state        = STATE_TITLE;
    g->frame_count  = 0;
    g->rand_seed    = 0xDEADBEEFu;
    g->screen_dirty = 0;
    g->score_dirty  = 0;
    g->shake_timer  = 0;
    g->level_clear_timer = 0;
    for (i = 0; i < MAX_BALLS;    i++) g->balls[i].active    = 0;
    for (i = 0; i < MAX_POWERUPS; i++) g->powerups[i].active = 0;
    for (i = 0; i < MAX_BULLETS;  i++) g->bullets[i].active  = 0;
    game_initLevel(g);
}

/* -----------------------------------------------------------------------
 * game_initLevel: reset ball/paddle/bricks for the current level
 * ----------------------------------------------------------------------- */
void game_initLevel(Game *g) {
    int row, col, i;
    const u8 (*layout)[8] = LEVEL_LAYOUTS[(g->level - 1) % NUM_LEVELS];
    const u16 *colors      = LEVEL_COLORS [(g->level - 1) % NUM_LEVELS];

    /* Reset effects */
    g->effects.wide_timer     = 0;
    g->effects.slow_timer     = 0;
    g->effects.laser_timer    = 0;
    g->effects.sticky_timer   = 0;
    g->effects.laser_cooldown = 0;
    g->effects.paddle_width   = PADDLE_WIDTH_NORMAL;

    /* Clear any active power-ups and bullets */
    for (i = 0; i < MAX_POWERUPS; i++) { g->powerups[i].active = 0; g->powerups[i].prev_y = -1; }
    for (i = 0; i < MAX_BULLETS;  i++) { g->bullets[i].active  = 0; g->bullets[i].prev_y  = -1; }

    /* Paddle */
    g->paddle.x        = (SCREEN_WIDTH - PADDLE_WIDTH_NORMAL) / 2;
    g->prev_paddle_x   = g->paddle.x;
    g->prev_paddle_width = PADDLE_WIDTH_NORMAL;

    /* Build bricks from layout */
    g->bricks_remaining = 0;
    for (row = 0; row < BRICK_ROWS; row++) {
        for (col = 0; col < BRICK_COLS; col++) {
            i = row * BRICK_COLS + col;
            u8 t = layout[row][col];
            g->bricks[i].x          = BRICK_ORIGIN_X + col * (BRICK_WIDTH  + BRICK_GAP);
            g->bricks[i].y          = BRICK_ORIGIN_Y + row * (BRICK_HEIGHT + BRICK_GAP);
            g->bricks[i].full_color = (t == BT_STEEL) ? COLOR_STEEL : colors[row];
            if (t == BT_STEEL) {
                g->bricks[i].max_hits  = 0;
                g->bricks[i].hits_left = 0;
            } else {
                g->bricks[i].max_hits  = t;   /* 0=empty handled below */
                g->bricks[i].hits_left = t;
                if (t >= 1) g->bricks_remaining++;
            }
        }
    }

    /* Reset ball (stuck to paddle) */
    {
        int bonus = (g->level - 1) * LEVEL_SPEED;
        game_reset_balls(g);
        g->balls[0].vx = BALL_BASE_VX + bonus;
        g->balls[0].vy = BALL_BASE_VY - bonus;
    }

    g->level_clear_timer = 0;
    g->score_dirty  = 1;
    g->screen_dirty = 1;
}

/* -----------------------------------------------------------------------
 * game_update: one frame of physics + input (STATE_PLAYING)
 * ----------------------------------------------------------------------- */
void game_update(Game *g, u16 keys, u16 keys_new) {
    int bi, i;
    int pw = g->effects.paddle_width;

    /* --- Level-clear cooldown --- */
    if (g->level_clear_timer > 0) {
        if (--g->level_clear_timer == 0) {
            if (g->level >= NUM_LEVELS) {
                g->state        = STATE_WIN;
                g->screen_dirty = 1;
            } else {
                g->level++;
                game_initLevel(g);
            }
        }
        return;
    }

    /* --- Move paddle --- */
    if (keys & KEY_LEFT) {
        g->paddle.x -= PADDLE_SPEED;
        if (g->paddle.x < 0) g->paddle.x = 0;
    }
    if (keys & KEY_RIGHT) {
        g->paddle.x += PADDLE_SPEED;
        if (g->paddle.x > SCREEN_WIDTH - pw)
            g->paddle.x = SCREEN_WIDTH - pw;
    }

    /* --- Fire laser (B button, edge-detected) --- */
    if ((keys_new & KEY_B) &&
        g->effects.laser_timer > 0 &&
        g->effects.laser_cooldown == 0) {
        int fired = 0;
        for (i = 0; i < MAX_BULLETS && fired < 2; i++) {
            if (!g->bullets[i].active) {
                g->bullets[i].active = 1;
                g->bullets[i].prev_y = -1;
                g->bullets[i].y      = PADDLE_Y - BULLET_H;
                g->bullets[i].x      = (fired == 0)
                    ? g->paddle.x + 2
                    : g->paddle.x + pw - BULLET_W - 2;
                fired++;
            }
        }
        if (fired > 0) {
            g->effects.laser_cooldown = LASER_COOLDOWN_FRAMES;
            sound_laser_fire();
        }
    }

    /* --- Move bullets + brick collision --- */
    for (i = 0; i < MAX_BULLETS; i++) {
        Bullet *bu = &g->bullets[i];
        if (!bu->active) continue;
        bu->y -= BULLET_SPEED;
        if (bu->y + BULLET_H < PLAY_TOP_Y) {
            bu->active = 0;
            continue;
        }
        /* Bullet-brick collision */
        {
            int j;
            for (j = 0; j < TOTAL_BRICKS; j++) {
                Brick *br = &g->bricks[j];
                if (!brick_is_alive(br)) continue;
                if (!aabb(bu->x, bu->y, BULLET_W, BULLET_H,
                          br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT)) continue;
                /* Hit! */
                bu->active = 0;
                if (br->max_hits == 0) break;  /* steel: destroy bullet, no damage */
                br->hits_left--;
                g->score += SCORE_PER_HIT;
                g->score_dirty = 1;
                if (br->hits_left == 0) {
                    gba_fillRect(br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT, COLOR_PLAY_BG);
                    g->bricks_remaining--;
                    try_drop_powerup(g, br);
                    sound_brick_break();
                    if (g->score > g->high_score) {
                        g->high_score = g->score;
                        sram_save_high_score(g->high_score);
                    }
                    if (g->bricks_remaining == 0) {
                        g->level_clear_timer = LEVEL_CLEAR_DELAY;
                        sound_level_clear();
                        return;
                    }
                } else {
                    gba_fillRect(br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT,
                                 brick_display_color(br));
                    sound_ball_bounce();
                }
                break;
            }
        }
    }

    /* --- Move power-ups + paddle catch --- */
    for (i = 0; i < MAX_POWERUPS; i++) {
        PowerUp *pu = &g->powerups[i];
        if (!pu->active) continue;
        pu->y_fp += POWERUP_FALL_SPEED;
        pu->y     = pu->y_fp / FP_SCALE;
        if (pu->y >= SCREEN_HEIGHT) { pu->active = 0; pu->prev_y = -1; continue; }
        /* Catch by paddle */
        if (pu->y + POWERUP_H >= PADDLE_Y &&
            pu->y  < PADDLE_Y + PADDLE_HEIGHT &&
            pu->x + POWERUP_W  > g->paddle.x &&
            pu->x  < g->paddle.x + pw) {
            game_apply_powerup(g, pu->type);
            pu->active = 0;
            pu->prev_y = -1;
            g->score_dirty = 1;
        }
    }

    /* --- Move each ball --- */
    for (bi = 0; bi < MAX_BALLS; bi++) {
        Ball *ball = &g->balls[bi];
        int   bpx, bpy;

        if (!ball->active) continue;

        /* Unlaunched ball tracks paddle */
        if (!ball->launched) {
            ball->x = (g->paddle.x + pw / 2 - BALL_SIZE / 2) * FP_SCALE;
            if (keys & KEY_A) ball->launched = 1;
            continue;
        }

        /* Apply slow factor */
        {
            int mvx = ball->vx;
            int mvy = ball->vy;
            if (g->effects.slow_timer > 0) { mvx = mvx * 2 / 3; mvy = mvy * 2 / 3; }
            ball->x += mvx;
            ball->y += mvy;
        }

        bpx = ball->x / FP_SCALE;
        bpy = ball->y / FP_SCALE;

        /* Wall collisions */
        if (ball->x < 0) {
            ball->x = 0; ball->vx = ABS(ball->vx);
            enforce_min_vx(ball); bpx = 0;
            sound_ball_bounce();
        }
        if (ball->x > (SCREEN_WIDTH - BALL_SIZE) * FP_SCALE) {
            ball->x = (SCREEN_WIDTH - BALL_SIZE) * FP_SCALE;
            ball->vx = -ABS(ball->vx);
            enforce_min_vx(ball); bpx = SCREEN_WIDTH - BALL_SIZE;
            sound_ball_bounce();
        }
        if (ball->y < PLAY_TOP_Y * FP_SCALE) {
            ball->y = PLAY_TOP_Y * FP_SCALE;
            ball->vy = ABS(ball->vy);
            bpy = PLAY_TOP_Y;
            sound_ball_bounce();
        }

        /* Ball off bottom */
        if (ball->y > SCREEN_HEIGHT * FP_SCALE) {
            ball->active = 0;
            g->ball_count--;
            continue;
        }

        /* Paddle collision (only if moving downward) */
        if (ball->vy > 0 &&
            aabb(bpx, bpy, BALL_SIZE, BALL_SIZE,
                 g->paddle.x, PADDLE_Y, pw, PADDLE_HEIGHT)) {
            if (bi == 0 && g->effects.sticky_timer > 0) {
                /* Sticky: re-attach ball to paddle */
                ball->launched = 0;
                ball->y = (PADDLE_Y - BALL_SIZE) * FP_SCALE;
            } else {
                int off = (bpx + BALL_SIZE / 2) - (g->paddle.x + pw / 2);
                ball->y  = (PADDLE_Y - BALL_SIZE) * FP_SCALE;
                ball->vy = -ABS(ball->vy);
                ball->vx += off * 4;
                ball->vx  = CLAMP(ball->vx, -MAX_SPEED, MAX_SPEED);
                enforce_min_vx(ball);
            }
            sound_ball_bounce();
            continue;
        }

        /* Brick collision */
        for (i = 0; i < TOTAL_BRICKS; i++) {
            Brick *br = &g->bricks[i];
            int pen_l, pen_r, pen_t, pen_b, mpx, mpy;

            if (!brick_is_alive(br)) continue;
            if (!aabb(bpx, bpy, BALL_SIZE, BALL_SIZE,
                      br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT)) continue;

            /* Penetration-depth axis selection */
            pen_l = (bpx + BALL_SIZE) - br->x;
            pen_r = (br->x + BRICK_WIDTH)  - bpx;
            pen_t = (bpy + BALL_SIZE) - br->y;
            pen_b = (br->y + BRICK_HEIGHT) - bpy;
            mpx   = MIN(pen_l, pen_r);
            mpy   = MIN(pen_t, pen_b);

            if (mpx < mpy) ball->vx = -ball->vx;
            else           ball->vy = -ball->vy;

            if (br->max_hits == 0) {
                /* Steel: reflect only */
                sound_ball_bounce();
            } else {
                br->hits_left--;
                g->score += SCORE_PER_HIT;
                g->score_dirty = 1;
                if (br->hits_left == 0) {
                    gba_fillRect(br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT, COLOR_PLAY_BG);
                    g->bricks_remaining--;
                    ball_boost(ball);
                    try_drop_powerup(g, br);
                    sound_brick_break();
                    if (g->score > g->high_score) {
                        g->high_score = g->score;
                        sram_save_high_score(g->high_score);
                    }
                    if (g->bricks_remaining == 0) {
                        g->level_clear_timer = LEVEL_CLEAR_DELAY;
                        sound_level_clear();
                        return;
                    }
                } else {
                    gba_fillRect(br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT,
                                 brick_display_color(br));
                    sound_ball_bounce();
                }
            }
            break;
        }
    }

    /* --- All balls lost? --- */
    if (g->ball_count == 0) {
        g->lives--;
        g->score_dirty = 1;
        g->shake_timer = 30;
        sound_life_lost();
        if (g->lives <= 0) {
            g->state        = STATE_GAMEOVER;
            g->screen_dirty = 1;
        } else {
            game_reset_balls(g);
            {
                int bonus = (g->level - 1) * LEVEL_SPEED;
                g->balls[0].vx = BALL_BASE_VX + bonus;
                g->balls[0].vy = BALL_BASE_VY - bonus;
            }
        }
        return;
    }

    update_effects(g);
}

/* -----------------------------------------------------------------------
 * game_drawHUD
 * ----------------------------------------------------------------------- */
void game_drawHUD(const Game *g) {
    int i;
    gba_fillRect(0, 0, SCREEN_WIDTH, HUD_HEIGHT, COLOR_HUD_BG);
    gba_drawHLine(0, HUD_SEP_Y, SCREEN_WIDTH, COLOR_SEP);

    gba_drawString(2, 2, "SCO:", COLOR_HUD_FG, COLOR_HUD_BG);
    gba_drawNumber(26, 2, g->score, COLOR_YELLOW, COLOR_HUD_BG);

    gba_drawString(82, 2, "HI:", COLOR_HUD_FG, COLOR_HUD_BG);
    gba_drawNumber(100, 2, g->high_score, COLOR_CYAN, COLOR_HUD_BG);

    gba_drawString(156, 2, "LV:", COLOR_HUD_FG, COLOR_HUD_BG);
    gba_drawNumber(174, 2, g->level, COLOR_WHITE, COLOR_HUD_BG);

    for (i = 0; i < g->lives; i++)
        gba_fillRect(196 + i * 9, 2, 7, 7, RGB15(28, 4, 4));
    for (i = g->lives; i < MAX_LIVES; i++)
        gba_fillRect(196 + i * 9, 2, 7, 7, COLOR_HUD_BG);
}

/* -----------------------------------------------------------------------
 * game_drawBricks
 * ----------------------------------------------------------------------- */
void game_drawBricks(const Game *g) {
    int i;
    for (i = 0; i < TOTAL_BRICKS; i++) {
        const Brick *br = &g->bricks[i];
        if (br->max_hits == 0) {           /* steel */
            gba_fillRect(br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT, COLOR_STEEL);
        } else if (br->hits_left > 0) {    /* breakable, alive */
            gba_fillRect(br->x, br->y, BRICK_WIDTH, BRICK_HEIGHT,
                         brick_display_color(br));
        }
        /* empty/destroyed: skip */
    }
}

/* -----------------------------------------------------------------------
 * game_draw: dirty-rectangle per-frame rendering
 * ----------------------------------------------------------------------- */
void game_draw(Game *g) {
    int i, bpx, bpy;

    /* Full redraw on new level or state entry */
    if (g->screen_dirty) {
        gba_clearScreen(COLOR_PLAY_BG);
        game_drawBricks(g);
        gba_fillRect(g->paddle.x, PADDLE_Y,
                     g->effects.paddle_width, PADDLE_HEIGHT,
                     get_paddle_color(&g->effects));
        g->prev_paddle_x     = g->paddle.x;
        g->prev_paddle_width = g->effects.paddle_width;
        for (i = 0; i < MAX_BALLS; i++) {
            Ball *b = &g->balls[i];
            if (!b->active) { b->prev_px = -1; continue; }
            bpx = b->x / FP_SCALE; bpy = b->y / FP_SCALE;
            gba_fillRect(bpx, bpy, BALL_SIZE, BALL_SIZE, COLOR_BALL);
            b->prev_px = bpx; b->prev_py = bpy;
        }
        for (i = 0; i < MAX_POWERUPS; i++) {
            PowerUp *pu = &g->powerups[i];
            if (!pu->active) { pu->prev_y = -1; continue; }
            draw_powerup(pu); pu->prev_y = pu->y;
        }
        for (i = 0; i < MAX_BULLETS; i++) {
            Bullet *bu = &g->bullets[i];
            if (!bu->active) { bu->prev_y = -1; continue; }
            gba_fillRect(bu->x, bu->y, BULLET_W, BULLET_H, COLOR_BULLET);
            bu->prev_y = bu->y;
        }
        game_drawHUD(g);
        g->screen_dirty = 0;
        g->score_dirty  = 0;
        return;
    }

    /* ----- Erase moving objects at old positions ----- */
    for (i = 0; i < MAX_BULLETS; i++) {
        Bullet *bu = &g->bullets[i];
        if (bu->prev_y < 0) continue;
        gba_fillRect(bu->x, bu->prev_y, BULLET_W, BULLET_H, COLOR_PLAY_BG);
        restore_bricks(g, bu->x, bu->prev_y, BULLET_W, BULLET_H);
    }
    for (i = 0; i < MAX_POWERUPS; i++) {
        PowerUp *pu = &g->powerups[i];
        if (pu->prev_y < 0) continue;
        gba_fillRect(pu->x, pu->prev_y, POWERUP_W, POWERUP_H, COLOR_PLAY_BG);
        restore_bricks(g, pu->x, pu->prev_y, POWERUP_W, POWERUP_H);
    }
    for (i = 0; i < MAX_BALLS; i++) {
        Ball *b = &g->balls[i];
        if (b->prev_px < 0) continue;
        gba_fillRect(b->prev_px, b->prev_py, BALL_SIZE, BALL_SIZE, COLOR_PLAY_BG);
        restore_bricks(g, b->prev_px, b->prev_py, BALL_SIZE, BALL_SIZE);
    }
    /* Erase old paddle */
    gba_fillRect(g->prev_paddle_x, PADDLE_Y,
                 g->prev_paddle_width, PADDLE_HEIGHT, COLOR_PLAY_BG);

    /* ----- Draw new paddle ----- */
    gba_fillRect(g->paddle.x, PADDLE_Y,
                 g->effects.paddle_width, PADDLE_HEIGHT,
                 get_paddle_color(&g->effects));
    g->prev_paddle_x     = g->paddle.x;
    g->prev_paddle_width = g->effects.paddle_width;

    /* ----- Draw moving objects at new positions ----- */
    for (i = 0; i < MAX_BULLETS; i++) {
        Bullet *bu = &g->bullets[i];
        if (!bu->active) { bu->prev_y = -1; continue; }
        gba_fillRect(bu->x, bu->y, BULLET_W, BULLET_H, COLOR_BULLET);
        bu->prev_y = bu->y;
    }
    for (i = 0; i < MAX_POWERUPS; i++) {
        PowerUp *pu = &g->powerups[i];
        if (!pu->active) { pu->prev_y = -1; continue; }
        draw_powerup(pu); pu->prev_y = pu->y;
    }
    for (i = 0; i < MAX_BALLS; i++) {
        Ball *b = &g->balls[i];
        if (!b->active) { b->prev_px = -1; continue; }
        bpx = b->x / FP_SCALE; bpy = b->y / FP_SCALE;
        gba_fillRect(bpx, bpy, BALL_SIZE, BALL_SIZE, COLOR_BALL);
        b->prev_px = bpx; b->prev_py = bpy;
    }

    /* ----- Level clear banner (drawn on top each frame) ----- */
    if (g->level_clear_timer > 0) {
        int bx = 40, by = 65, bw = 160, bh = 30;
        gba_fillRect(bx, by, bw, bh, RGB15(3, 3, 8));
        gba_drawRect(bx, by, bw, bh, COLOR_WHITE);
        {
            const char *s = "LEVEL CLEAR!";
            int tx = bx + (bw - gba_stringWidth(s)) / 2;
            gba_drawString(tx, by + 11, s, COLOR_YELLOW, RGB15(3, 3, 8));
        }
    }

    /* ----- HUD ----- */
    if (g->score_dirty) {
        game_drawHUD(g);
        g->score_dirty = 0;
    }
}

/* -----------------------------------------------------------------------
 * Title screen
 * ----------------------------------------------------------------------- */
void draw_titleScreen(Game *g) {
    int i, x;
    const char *title = "BRICK BREAKER GBA";
    const char *sub   = "A GBA BREAKOUT CLONE";
    const char *inst1 = "D-PAD: MOVE PADDLE";
    const char *inst2 = "A: LAUNCH   B: LASER";
    const char *inst3 = "START: PAUSE";

    gba_clearScreen(COLOR_BLACK);

    x = (SCREEN_WIDTH - gba_stringWidth(title)) / 2;
    gba_drawString(x + 1, 23, title, COLOR_SHADOW, COLOR_BLACK);
    gba_drawString(x,     22, title, COLOR_TITLE,  COLOR_BLACK);

    x = (SCREEN_WIDTH - gba_stringWidth(sub)) / 2;
    gba_drawString(x, 38, sub, COLOR_GRAY, COLOR_BLACK);

    /* Mini brick grid */
    for (i = 0; i < BRICK_ROWS; i++) {
        int bx;
        u16 c = LEVEL_COLORS[0][i];
        for (bx = 0; bx < 8; bx++)
            gba_fillRect(4 + bx * 29, 50 + i * 8, 28, 7, c);
    }

    if (g->high_score > 0) {
        const char *hi = "BEST:";
        x = (SCREEN_WIDTH - gba_stringWidth(hi) - 6 * 30) / 2;
        gba_drawString(x, 108, hi, COLOR_CYAN, COLOR_BLACK);
        gba_drawNumber(x + gba_stringWidth(hi) + 2, 108,
                       g->high_score, COLOR_YELLOW, COLOR_BLACK);
    }

    x = (SCREEN_WIDTH - gba_stringWidth(inst1)) / 2;
    gba_drawString(x, 124, inst1, COLOR_GRAY, COLOR_BLACK);
    x = (SCREEN_WIDTH - gba_stringWidth(inst2)) / 2;
    gba_drawString(x, 134, inst2, COLOR_GRAY, COLOR_BLACK);
    x = (SCREEN_WIDTH - gba_stringWidth(inst3)) / 2;
    gba_drawString(x, 144, inst3, COLOR_GRAY, COLOR_BLACK);

    g->frame_count = 0;
}

/* -----------------------------------------------------------------------
 * Flashing "PRESS START" helper
 * ----------------------------------------------------------------------- */
static void flash_press_start(u32 frame_count, int y, u16 bg) {
    const char *msg = "PRESS START";
    int x = (SCREEN_WIDTH - gba_stringWidth(msg)) / 2;
    u16 color = ((frame_count % 60) < 30) ? COLOR_WHITE : bg;
    gba_drawString(x, y, msg, color, bg);
}

void game_flashPressStart(u32 frame_count, int y, u16 bg) {
    flash_press_start(frame_count, y, bg);
}

/* -----------------------------------------------------------------------
 * Pause overlay
 * ----------------------------------------------------------------------- */
void draw_pauseOverlay(void) {
    int bx = 60, by = 60, bw = 120, bh = 40;
    gba_fillRect(bx, by, bw, bh, RGB15(4, 4, 8));
    gba_drawRect(bx, by, bw, bh, COLOR_WHITE);
    {
        const char *p = "PAUSED";
        int x = bx + (bw - gba_stringWidth(p)) / 2;
        gba_drawString(x, by + 8,  p, COLOR_WHITE, RGB15(4, 4, 8));
    }
    {
        const char *p = "START TO RESUME";
        int x = bx + (bw - gba_stringWidth(p)) / 2;
        gba_drawString(x, by + 24, p, COLOR_CYAN,  RGB15(4, 4, 8));
    }
}

/* -----------------------------------------------------------------------
 * Game over screen
 * ----------------------------------------------------------------------- */
void draw_gameOverScreen(const Game *g) {
    const char *go    = "GAME OVER";
    const char *sc    = "SCORE:";
    const char *hi    = "BEST:";
    const char *retry = "START  TO RETRY";
    const char *back  = "SELECT TO TITLE";
    int x;

    gba_clearScreen(COLOR_BLACK);

    x = (SCREEN_WIDTH - gba_stringWidth(go)) / 2;
    gba_drawString(x + 1, 46, go, COLOR_SHADOW, COLOR_BLACK);
    gba_drawString(x,     45, go, COLOR_RED,    COLOR_BLACK);

    x = (SCREEN_WIDTH - gba_stringWidth(sc) - 6 * 5) / 2;
    gba_drawString(x, 68, sc, COLOR_WHITE, COLOR_BLACK);
    gba_drawNumber(x + gba_stringWidth(sc) + 2, 68,
                   g->score, COLOR_YELLOW, COLOR_BLACK);

    x = (SCREEN_WIDTH - gba_stringWidth(hi) - 6 * 5) / 2;
    gba_drawString(x, 80, hi, COLOR_HUD_FG, COLOR_BLACK);
    gba_drawNumber(x + gba_stringWidth(hi) + 2, 80,
                   g->high_score, COLOR_CYAN, COLOR_BLACK);

    x = (SCREEN_WIDTH - gba_stringWidth(retry)) / 2;
    gba_drawString(x, 102, retry, COLOR_GRAY, COLOR_BLACK);
    x = (SCREEN_WIDTH - gba_stringWidth(back)) / 2;
    gba_drawString(x, 114, back,  COLOR_GRAY, COLOR_BLACK);
}

/* -----------------------------------------------------------------------
 * Win screen
 * ----------------------------------------------------------------------- */
void draw_winScreen(const Game *g) {
    const char *win  = "YOU WIN!";
    const char *sc   = "FINAL SCORE:";
    int i, x;

    gba_clearScreen(RGB15(0, 0, 4));

    for (i = 0; i < BRICK_ROWS; i++)
        gba_fillRect(4 + i * 39, 28, 36, 10, LEVEL_COLORS[0][i]);

    x = (SCREEN_WIDTH - gba_stringWidth(win)) / 2;
    gba_drawString(x + 1, 55, win, COLOR_SHADOW, RGB15(0, 0, 4));
    gba_drawString(x,     54, win, COLOR_YELLOW, RGB15(0, 0, 4));

    x = (SCREEN_WIDTH - gba_stringWidth(sc)) / 2;
    gba_drawString(x, 76, sc, COLOR_WHITE, RGB15(0, 0, 4));
    gba_drawNumber(x + gba_stringWidth(sc) + 2, 76,
                   g->score, COLOR_YELLOW, RGB15(0, 0, 4));

    (void)i;
}
