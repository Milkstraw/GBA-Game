#ifndef NPC_H
#define NPC_H

#include "gba_types.h"
#include "constants.h"

/* NPC state machine */
typedef enum {
    NPC_IDLE     = 0,
    NPC_NAVIGATE = 1,
    NPC_AT_SHELF = 2,
    NPC_QUEUE    = 3,
    NPC_PAYING   = 4,
    NPC_EXITING  = 5
} NpcState;

/*
 * A waypoint is a (col, row) tile coordinate pair.
 * Sentinel: x==0xFF marks end of route array.
 */
typedef struct {
    u8 x;
    u8 y;
} WayPoint;

typedef struct {
    u8  x, y;              /* tile position */
    u8  px, py;            /* pixel position */
    u8  dir;               /* DIR_DOWN/UP/LEFT/RIGHT */
    u8  state;             /* NpcState */
    u8  shopping_list[3];  /* product IDs; 0xFF = empty slot */
    u8  list_size;
    u8  list_index;        /* index of item currently being sought */
    u8  cart_count;        /* items picked up so far */
    u16 cart_total;        /* cents owed at checkout */
    u8  patience;          /* in-game minutes left before abandoning queue */
    u8  wp_index;          /* index into active waypoint route */
    u8  move_timer;        /* frames until next tile move (max 24) */
    u8  anim_frame;
    u8  anim_timer;
    u8  active;            /* 1 = slot in use */
} NPC;

/* Global NPC pool */
extern NPC g_npcs[MAX_NPCS];

/* Set to 1 when an NPC is at the register waiting for the player */
extern u8 g_customer_waiting;

/* Tracks last minute used for patience ticks; update once before npc_update loop. */
extern u8 g_npc_patience_minute;

/* Points to the NPC currently at the register (or NULL) */
extern NPC *g_npc_at_register;

/* Spawn timer: counts down frames between NPC spawns */
extern u16 g_npc_spawn_timer;

/* How many frames between spawns (2700 ≈ 45s at 60fps) */
#define NPC_SPAWN_INTERVAL  2700u
#define NPC_MOVE_PERIOD     24u    /* frames per tile move */
#define NPC_PATIENCE_START  5u     /* 5 in-game minutes */
#define NPC_ANIM_PERIOD     8u

/* Clear all NPC slots. */
void npc_init(void);

/*
 * Find a free slot, place NPC at door tile (7,9), generate shopping list,
 * begin navigating to first shelf.  Called when spawn timer fires.
 */
void npc_spawn(void);

/* Advance a single NPC's state machine by one frame. */
void npc_update(NPC *n);

/*
 * Complete checkout for npc: add cart_total to g_cash, set NPC to EXITING.
 * Clears g_customer_waiting and g_npc_at_register.
 */
void npc_checkout(NPC *n);

/* Write OAM entries for this NPC (body = entries 2+3, cart = entry 4). */
void npc_draw(NPC *n);

#endif /* NPC_H */
