#include "input.h"
#include "../gba.h"

static u16 key_curr = 0;
static u16 key_prev = 0;

void key_poll(void) {
    key_prev = key_curr;
    key_curr = (~REG_KEYINPUT) & 0x03FF;
}

u16 key_held(u16 mask) {
    return key_curr & mask;
}

u16 key_pressed(u16 mask) {
    return (key_curr & ~key_prev) & mask;
}
