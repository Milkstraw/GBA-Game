#ifndef GAME_H
#define GAME_H

#include "gba.h"

/* -----------------------------------------------------------------------
 * Screen layout
 * ----------------------------------------------------------------------- */
#define HUD_HEIGHT    12
#define HUD_SEP_Y     12
#define PLAY_TOP_Y    13

#define PADDLE_Y             148
#define PADDLE_WIDTH_NORMAL  40
#define PADDLE_WIDTH_WIDE    80
#define PADDLE_HEIGHT        5
#define PADDLE_SPEED         3

#define BALL_SIZE     4

/* -----------------------------------------------------------------------
 * Fixed-point (×256)
 * ----------------------------------------------------------------------- */
#define FP_SCALE      256

#define BALL_BASE_VX  192
#define BALL_BASE_VY  (-320)
#define LEVEL_SPEED   32         /* extra fp/frame per level above 1 */
#define SPEED_BUMP    4
#define MAX_SPEED     640
#define MIN_VX        64

/* -----------------------------------------------------------------------
 * Brick grid
 * ----------------------------------------------------------------------- */
#define BRICK_COLS     8
#define BRICK_ROWS     6
#define BRICK_WIDTH    28
#define BRICK_HEIGHT   10
#define BRICK_GAP      1
#define BRICK_ORIGIN_X 4
#define BRICK_ORIGIN_Y 14
#define TOTAL_BRICKS   (BRICK_COLS * BRICK_ROWS)

/* -----------------------------------------------------------------------
 * Scoring / game rules
 * ----------------------------------------------------------------------- */
#define SCORE_PER_HIT   10      /* points per hit on any breakable brick */
#define LIVES_START     3
#define MAX_LIVES       5
#define NUM_LEVELS      8       /* must match levels.h */

/* -----------------------------------------------------------------------
 * Power-ups
 * ----------------------------------------------------------------------- */
#define MAX_POWERUPS          4
#define POWERUP_W             16
#define POWERUP_H             10
#define POWERUP_FALL_SPEED    128  /* fp: ~0.5 px/frame */

#define EFFECT_WIDE_DURATION   600
#define EFFECT_SLOW_DURATION   360
#define EFFECT_LASER_DURATION  480
#define EFFECT_STICKY_DURATION 360
#define LASER_COOLDOWN_FRAMES  20

/* -----------------------------------------------------------------------
 * Multi-ball / bullets
 * ----------------------------------------------------------------------- */
#define MAX_BALLS    3
#define MAX_BULLETS  4
#define BULLET_W     2
#define BULLET_H     8
#define BULLET_SPEED 3   /* px/frame upward */

/* -----------------------------------------------------------------------
 * Misc
 * ----------------------------------------------------------------------- */
#define LEVEL_CLEAR_DELAY  90   /* frames of "LEVEL CLEAR!" banner */

/* -----------------------------------------------------------------------
 * Color palette (BGR555)
 * ----------------------------------------------------------------------- */
#define COLOR_BLACK    RGB15( 0,  0,  0)
#define COLOR_WHITE    RGB15(31, 31, 31)
#define COLOR_GRAY     RGB15(15, 15, 15)
#define COLOR_DARKGRAY RGB15( 6,  6,  6)
#define COLOR_HUD_BG   RGB15( 3,  3,  6)
#define COLOR_HUD_FG   RGB15(26, 26, 31)
#define COLOR_PLAY_BG  RGB15( 1,  1,  3)
#define COLOR_PADDLE   RGB15(26, 26, 28)
#define COLOR_BALL     RGB15(31, 31, 31)
#define COLOR_SEP      RGB15(10, 10, 14)
#define COLOR_STEEL    RGB15(18, 18, 20)
#define COLOR_BULLET   RGB15(31, 28, 10)
#define COLOR_YELLOW   RGB15(31, 28,  0)
#define COLOR_CYAN     RGB15( 0, 28, 28)
#define COLOR_RED      RGB15(31,  0,  0)
#define COLOR_GREEN    RGB15( 0, 26,  0)
#define COLOR_BLUE     RGB15( 0,  0, 31)
#define COLOR_TITLE    RGB15(16, 24, 31)
#define COLOR_SHADOW   RGB15( 4,  8, 12)

/* -----------------------------------------------------------------------
 * Structs
 * ----------------------------------------------------------------------- */

typedef enum {
    POWERUP_WIDE = 1,
    POWERUP_MULTIBALL,
    POWERUP_SLOW,
    POWERUP_LASER,
    POWERUP_STICKY,
    POWERUP_LIFE,
    POWERUP_COUNT
} PowerUpType;

typedef struct {
    int  x, y;        /* fixed-point position */
    int  vx, vy;      /* fixed-point velocity */
    int  launched;    /* 0 = stuck to paddle (only balls[0]) */
    int  active;      /* 1 = in play */
    int  prev_px;     /* -1 means not yet drawn */
    int  prev_py;
} Ball;

typedef struct {
    int  x;           /* left edge pixel (not fixed-point) */
} Paddle;

typedef struct {
    int  x, y;        /* pixel top-left */
    u16  full_color;  /* color at maximum health */
    int  hits_left;   /* remaining hits; ignored for steel */
    int  max_hits;    /* 0=steel (indestructible), 1-3=breakable */
} Brick;

typedef struct {
    int  x;           /* pixel x (fixed across fall) */
    int  y;           /* pixel y (current) */
    int  y_fp;        /* fixed-point y accumulator */
    int  prev_y;      /* -1 if not drawn yet */
    int  type;        /* PowerUpType */
    int  active;
} PowerUp;

typedef struct {
    int  x, y;        /* pixel top-left */
    int  prev_y;      /* -1 if not drawn */
    int  active;
} Bullet;

typedef struct {
    int  wide_timer;
    int  slow_timer;
    int  laser_timer;
    int  sticky_timer;
    int  laser_cooldown;
    int  paddle_width;  /* current effective paddle width */
} Effects;

typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER,
    STATE_WIN
} GameState;

typedef struct {
    Ball      balls[MAX_BALLS];
    int       ball_count;
    Paddle    paddle;
    Brick     bricks[TOTAL_BRICKS];
    PowerUp   powerups[MAX_POWERUPS];
    Bullet    bullets[MAX_BULLETS];
    Effects   effects;
    int       score;
    int       high_score;
    int       lives;
    int       bricks_remaining;
    GameState state;
    int       level;
    int       prev_paddle_x;
    int       prev_paddle_width;
    int       score_dirty;
    int       screen_dirty;
    int       level_clear_timer;
    int       shake_timer;
    u32       rand_seed;
    u32       frame_count;
} Game;

/* -----------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------- */
void game_init(Game *g);
void game_initLevel(Game *g);
void game_update(Game *g, u16 keys, u16 keys_new);
void game_draw(Game *g);
void game_drawHUD(const Game *g);
void game_drawBricks(const Game *g);
void update_shake(Game *g);

void draw_titleScreen(Game *g);
void draw_pauseOverlay(void);
void draw_gameOverScreen(const Game *g);
void draw_winScreen(const Game *g);
void game_flashPressStart(u32 frame_count, int y, u16 bg);

#endif /* GAME_H */
