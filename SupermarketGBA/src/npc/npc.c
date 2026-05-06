#include <tonc.h>
#include "npc.h"
#include "inventory.h"
#include "daytimer.h"
#include "player.h"
#include "tile_engine.h"
#include "demo_map.h"
#include "constants.h"

NPC  g_npcs[MAX_NPCS];
u8   g_customer_waiting    = 0;
NPC *g_npc_at_register     = NULL;
u16  g_npc_spawn_timer     = 0;
u8   g_npc_patience_minute = 0;  /* last minute value used for patience tick */

/* ----------------------------------------------------------------
 * Hardcoded waypoint routes (tile coords, sentinel x=0xFF at end)
 *
 * All routes start from the NPC's current position implicitly;
 * each array is a list of waypoints to visit in order.
 * ---------------------------------------------------------------- */

/* Door spawn → approach Shelf 1 (col 4, row 3) */
static const WayPoint ROUTE_SHELF1[] = {
    {7,8}, {6,8}, {5,8}, {4,8}, {4,7}, {4,6}, {4,5}, {4,4}, {4,3},
    {0xFF,0}
};

/* Current pos → Shelf 2 (col 9, row 3).  NPC walks across aisle. */
static const WayPoint ROUTE_SHELF2[] = {
    {4,3}, {5,3}, {6,3}, {7,3}, {8,3}, {9,3},
    {0xFF,0}
};

/* Any pos → Register (col 7, row 7 — stands south of register tile) */
static const WayPoint ROUTE_REGISTER[] = {
    {9,3}, {9,4}, {9,5}, {9,6}, {8,7}, {7,7},
    {0xFF,0}
};
/* Shorter path from shelf1 side to register */
static const WayPoint ROUTE_REGISTER_FROM_S1[] = {
    {4,3}, {4,4}, {4,5}, {4,6}, {5,7}, {6,7}, {7,7},
    {0xFF,0}
};

/* Register → exit door */
static const WayPoint ROUTE_EXIT[] = {
    {7,7}, {7,8}, {7,9},
    {0xFF,0}
};

/* ----------------------------------------------------------------
 * Minimal LCG pseudo-random generator (no floats, no libc rand)
 * ---------------------------------------------------------------- */
static u32 s_lcg_seed = 12345u;

static u8 lcg_rand8(void)
{
    s_lcg_seed = s_lcg_seed * 1664525u + 1013904223u;
    return (u8)(s_lcg_seed >> 24);
}

/* ----------------------------------------------------------------
 * npc_init
 * ---------------------------------------------------------------- */
void npc_init(void)
{
    u8 i;
    for (i = 0; i < MAX_NPCS; i++) {
        g_npcs[i].active = 0;
    }
    g_customer_waiting = 0;
    g_npc_at_register  = NULL;
    g_npc_spawn_timer  = NPC_SPAWN_INTERVAL;

    /* Seed LCG from day + minute so different days vary */
    s_lcg_seed = (u32)((u16)g_daytimer.day * 317u + g_daytimer.minute + 1u);
}

/* ----------------------------------------------------------------
 * npc_spawn
 * ---------------------------------------------------------------- */
void npc_spawn(void)
{
    u8 i;
    NPC *n = NULL;

    /* Find a free slot */
    for (i = 0; i < MAX_NPCS; i++) {
        if (!g_npcs[i].active) { n = &g_npcs[i]; break; }
    }
    if (!n) return;  /* no free slot */

    /* Place NPC at spawn tile (7,9) */
    n->x            = 7;
    n->y            = 9;
    n->px           = 7 * TILE_W;
    n->py           = 9 * TILE_H;
    n->dir          = DIR_UP;
    n->state        = NPC_NAVIGATE;
    n->cart_count   = 0;
    n->cart_total   = 0;
    n->patience     = NPC_PATIENCE_START;
    n->wp_index     = 0;
    n->move_timer   = 0;
    n->anim_frame   = 0;
    n->anim_timer   = 0;
    n->active       = 1;

    /* Generate shopping list (1-3 items from products 0-2) */
    n->list_size  = (u8)(1u + (lcg_rand8() % 3u));
    n->list_index = 0;
    for (i = 0; i < 3; i++) {
        n->shopping_list[i] = 0xFF;
    }
    for (i = 0; i < n->list_size; i++) {
        n->shopping_list[i] = (u8)(lcg_rand8() % 3u); /* products 0-2 */
    }

    /* Always start by heading to Shelf 1 */
    n->wp_index = 0;
}

/* ----------------------------------------------------------------
 * Get the active route for this NPC based on state and list position
 * ---------------------------------------------------------------- */
static const WayPoint *npc_get_route(NPC *n)
{
    if (n->state == NPC_EXITING)  return ROUTE_EXIT;
    if (n->state == NPC_QUEUE || n->state == NPC_PAYING) {
        /* Route to register from wherever the NPC currently is */
        if (n->x <= 6) return ROUTE_REGISTER_FROM_S1;
        return ROUTE_REGISTER;
    }
    /* Determine destination shelf for current list item */
    if (n->list_index < n->list_size) {
        u8 pid    = n->shopping_list[n->list_index];
        u8 shelf  = inventory_shelf_for_product(pid);
        if (shelf == 0) return ROUTE_SHELF1;
        if (shelf == 1) return ROUTE_SHELF2;
    }
    return ROUTE_EXIT;  /* fallback */
}

/* ----------------------------------------------------------------
 * npc_update — advance one NPC by one frame
 * ---------------------------------------------------------------- */
void npc_update(NPC *n)
{
    const WayPoint *route;
    WayPoint        target;

    if (!n->active) return;

    /* Patience: decrements each in-game minute while queuing.
     * g_npc_patience_minute is updated once per npc_update_all pass. */
    if (n->state == NPC_QUEUE) {
        if (g_daytimer.minute != g_npc_patience_minute) {
            if (n->patience > 0) n->patience--;
            if (n->patience == 0) {
                /* NPC abandons queue */
                n->active = 0;
                if (g_npc_at_register == n) {
                    g_npc_at_register = NULL;
                    g_customer_waiting = 0;
                }
                return;
            }
        }
    }

    /* PAYING: wait for player to call npc_checkout */
    if (n->state == NPC_PAYING) return;

    /* Get active waypoint route */
    route = npc_get_route(n);
    if (route[n->wp_index].x == 0xFF) {
        /* End of route — take action for current state */
        if (n->state == NPC_EXITING) {
            n->active = 0;
            return;
        }

        if (n->state == NPC_NAVIGATE) {
            /* Arrived at shelf — attempt to pick up item */
            u8 pid = (n->list_index < n->list_size)
                     ? n->shopping_list[n->list_index]
                     : 0xFF;
            if (pid != 0xFF && g_products[pid].stock_on_shelf > 0) {
                /* Item available */
                inventory_sell(pid);
                n->cart_count++;
                n->cart_total = (u16)(n->cart_total + g_products[pid].shelf_price);
            }
            /* Whether found or not, advance list */
            n->list_index++;
            n->wp_index = 0;  /* reset for next route */

            if (n->list_index >= n->list_size) {
                /* Done shopping — head to register */
                n->state = NPC_QUEUE;
                n->wp_index = 0;
            }
            return;
        }

        if (n->state == NPC_QUEUE) {
            /* Arrived at register tile */
            n->state          = NPC_PAYING;
            g_npc_at_register = n;
            g_customer_waiting = 1;
            return;
        }

        return;
    }

    /* Move toward current waypoint, 1 tile per NPC_MOVE_PERIOD frames */
    n->move_timer++;
    if (n->move_timer < NPC_MOVE_PERIOD) {
        /* Animate while waiting to move */
        n->anim_timer++;
        if (n->anim_timer >= NPC_ANIM_PERIOD) {
            n->anim_timer = 0;
            n->anim_frame = (u8)((n->anim_frame + 1u) % 3u);
        }
        return;
    }
    n->move_timer = 0;

    target = route[n->wp_index];

    /* One step toward target */
    if (n->x < target.x)      { n->x++; n->dir = DIR_RIGHT; }
    else if (n->x > target.x) { n->x--; n->dir = DIR_LEFT;  }
    else if (n->y < target.y) { n->y++; n->dir = DIR_DOWN;  }
    else if (n->y > target.y) { n->y--; n->dir = DIR_UP;    }
    else {
        /* Reached this waypoint — advance to next */
        n->wp_index++;
    }

    n->px = (u8)(n->x * TILE_W);
    n->py = (u8)(n->y * TILE_H);
}

/* ----------------------------------------------------------------
 * npc_checkout — called by checkout.c when player presses A
 * ---------------------------------------------------------------- */
void npc_checkout(NPC *n)
{
    if (!n) return;
    g_cash = (u16)(g_cash + n->cart_total);
    n->state      = NPC_EXITING;
    n->wp_index   = 0;
    g_customer_waiting = 0;
    g_npc_at_register  = NULL;
}

/* ----------------------------------------------------------------
 * npc_draw — OAM entries: body=2+3, cart=4
 * Cart tile: 0 items→frame0, 1 item→frame1, 2+→frame2
 * ---------------------------------------------------------------- */
void npc_draw(NPC *n)
{
    u16 body_tile, cart_tile;
    u16 px16, py16;
    u8  cart_frame;

    if (!n->active) return;

    /* NPC body sprite base tile: same layout as player but offset in sheet.
     * NPC uses tiles starting at PLAYER_TILES_PER_FRAME*12 (after 4 dirs×3 frames). */
#define NPC_SPR_BASE       72u  /* after player's 72 tiles */
#define NPC_TILES_PER_FRAME 6u

    body_tile = (u16)(NPC_SPR_BASE
                      + ((u16)n->dir * 3u + n->anim_frame)
                        * NPC_TILES_PER_FRAME);

    px16 = (u16)(n->px & 0x1FFu);
    py16 = (u16)(n->py & 0xFFu);

    /* OAM 2 — NPC upper 16×16 */
    obj_mem[2].attr0 = py16 | ATTR0_SQUARE;
    obj_mem[2].attr1 = px16 | ATTR1_SIZE_16;
    obj_mem[2].attr2 = body_tile | ATTR2_PALBANK(1);  /* SPR PAL 1 */

    /* OAM 3 — NPC lower 16×8 */
    obj_mem[3].attr0 = (u16)((n->py + 16u) & 0xFFu) | ATTR0_WIDE;
    obj_mem[3].attr1 = px16;
    obj_mem[3].attr2 = (u16)(body_tile + 4u) | ATTR2_PALBANK(1);

    /* OAM 4 — shopping cart (16×16, offset +8px right) */
    cart_frame = (n->cart_count == 0) ? 0u
               : (n->cart_count == 1) ? 1u : 2u;
#define CART_SPR_BASE  144u  /* after NPC tiles */
    cart_tile = (u16)(CART_SPR_BASE + cart_frame * 4u);

    obj_mem[4].attr0 = py16 | ATTR0_SQUARE;
    obj_mem[4].attr1 = (u16)((n->px + 8u) & 0x1FFu) | ATTR1_SIZE_16;
    obj_mem[4].attr2 = cart_tile | ATTR2_PALBANK(5);  /* SPR PAL 5 */
}
