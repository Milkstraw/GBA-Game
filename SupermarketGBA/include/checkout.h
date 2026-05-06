#ifndef CHECKOUT_H
#define CHECKOUT_H

#include "gba_types.h"

/*
 * Attempt to start checkout.  Call when player presses A facing the register.
 *
 * If g_customer_waiting == 0: shows "NO CUSTOMER" for 30 frames then returns.
 * If g_customer_waiting == 1: shows the checkout overlay (item list + total),
 *   waits for player to press A, calls npc_checkout, shows "+$X.XX" float
 *   animation for 60 frames, then restores BG1 and returns.
 *
 * Does NOT block the main loop for the full animation; the caller is
 * responsible for continuing to call hud_draw() / npc_draw() during the wait.
 * For the demo this is implemented as a simple blocking loop using VBlankIntrWait.
 */
void checkout_start(void);

#endif /* CHECKOUT_H */
