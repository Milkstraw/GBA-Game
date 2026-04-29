#ifndef GAME_H
#define GAME_H

#include "gba.h"

/* -----------------------------------------------------------------------
 * Screen layout
 * ----------------------------------------------------------------------- */
#define HUD_HEIGHT    12          /* rows 0-11 */
#define HUD_SEP_Y     12          /* 1-pixel separator line */
#define PLAY_TOP_Y    13          /* first row of play area */

#define PADDLE_Y      148
#define PADDLE_WIDTH  40
#define PADDLE_HEIGHT 5
#define PADDLE_SPEED  3           /* pixels per frame (whole pixels) */

#define BALL_SIZE     4           /* pixels square */

/* -----------------------------------------------------------------------
 * Fixed-point scale: positions/velocities stored as int * FP_SCALE.
 * Pixel pos = value / FP_SCALE.
 * ----------------------------------------------------------------------- */
#define FP_SCALE      256

/* Base velocities (fixed-point). vx is positive (rightward). vy is negative (upward). */
#define BALL_BASE_VX  192         /* 192/256 = 0.75 px/frame */
#define BALL_BASE_VY  (-320)      /* -320/256 = -1.25 px/frame */

/* Per-level speed increment for base velocity (added to both |vx| and |vy|) */
#define LEVEL_SPEED   32          /* 32/256 = 0.125 px/frame per level above 1 */

/* Per-brick speed boost (applied after each brick is broken) */
#define SPEED_BUMP    4           /* fixed-point units */
#define MAX_SPEED     640         /* 640/256 = 2.5 px/frame */
#define MIN_VX        64          /* prevent purely-vertical ball */

/* -----------------------------------------------------------------------
 * Brick grid
 * ----------------------------------------------------------------------- */
#define BRICK_COLS    8
#define BRICK_ROWS    6
#define BRICK_WIDTH   28
#define BRICK_HEIGHT  10
#define BRICK_GAP     1
#define BRICK_ORIGIN_X 4
#define BRICK_ORIGIN_Y 14
#define TOTAL_BRICKS  (BRICK_COLS * BRICK_ROWS)

/* -----------------------------------------------------------------------
 * Scoring / game rules
 * ----------------------------------------------------------------------- */
#define SCORE_PER_BRICK 10
#define LIVES_START     3
#define MAX_LEVEL       5

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
#define COLOR_YELLOW   RGB15(31, 28,  0)
#define COLOR_CYAN     RGB15( 0, 28, 28)
#define COLOR_RED      RGB15(31,  0,  0)
#define COLOR_ORANGE   RGB15(31, 14,  0)
#define COLOR_GREEN    RGB15( 0, 28,  0)
#define COLOR_BLUE     RGB15( 0,  0, 31)
#define COLOR_TITLE    RGB15(16, 24, 31)  /* light blue for title */
#define COLOR_SHADOW   RGB15( 4,  8, 12)  /* dark shadow for title */

/* Row 0 = topmost (hardest to reach), row 5 = bottommost (easiest) */
extern const u16 BRICK_COLORS[BRICK_ROWS];

/* -----------------------------------------------------------------------
 * Structs
 * ----------------------------------------------------------------------- */
typedef struct {
    int x, y;       /* fixed-point position */
    int vx, vy;     /* fixed-point velocity */
    int launched;   /* 0 = stuck to paddle */
} Ball;

typedef struct {
    int x;          /* left edge, pixel coords (not fixed-point) */
} Paddle;

typedef struct {
    int  x, y;      /* top-left pixel coords */
    u16  color;
    int  alive;
} Brick;

typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEOVER,
    STATE_WIN
} GameState;

typedef struct {
    Ball      ball;
    Paddle    paddle;
    Brick     bricks[TOTAL_BRICKS];
    int       score;
    int       lives;
    int       bricks_remaining;
    GameState state;
    int       level;
    int       prev_ball_px;
    int       prev_ball_py;
    int       prev_paddle_x;
    int       score_dirty;
    int       screen_dirty;
    u32       frame_count;
} Game;

/* -----------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------- */
void game_init(Game *g);
void game_initLevel(Game *g);
void game_update(Game *g, u16 keys);
void game_draw(Game *g);
void game_drawHUD(const Game *g);
void game_drawBricks(const Game *g);

void draw_titleScreen(Game *g);
void draw_pauseOverlay(void);
void draw_gameOverScreen(const Game *g);
void draw_winScreen(const Game *g);
void game_flashPressStart(u32 frame_count, int y, u16 bg);

#endif /* GAME_H */
