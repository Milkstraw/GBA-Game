#pragma once
#include "types.h"

/* Call once per frame before reading key state. */
void key_poll(void);

/* Returns non-zero if all bits in mask are currently held. */
u16 key_held(u16 mask);

/* Returns non-zero if all bits in mask were just pressed this frame. */
u16 key_pressed(u16 mask);
