/* Task 0.1 — stub entry point. Replace with full game loop in later tasks. */
#include "gba.h"
#include "core/input.h"

int main(void) {
    REG_DISPCNT = MODE0 | BG0_ENABLE;

    while (1) {
        vsync();
        key_poll();
    }

    return 0;
}
