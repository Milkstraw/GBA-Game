#include "input.h"

u16 keys_held     = 0;
u16 keys_pressed  = 0;
u16 keys_released = 0;

void input_poll(void) {
    u16 prev      = keys_held;
    u16 curr      = KEYS_RAW();
    keys_held     = curr;
    keys_pressed  = curr & ~prev;
    keys_released = ~curr & prev;
}
