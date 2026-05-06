#ifndef INVENTORY_SCREEN_H
#define INVENTORY_SCREEN_H

#include "gba_types.h"

/*
 * Open the full-screen inventory overlay on BG1.
 * Blocks until player presses B to close.
 * Saves and restores BG1 screenblock content on open/close.
 */
void inventory_screen_open(void);

#endif /* INVENTORY_SCREEN_H */
