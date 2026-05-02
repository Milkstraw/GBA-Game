#include "endgame.h"
#include "debug.h"
#include "../gba.h"

static bool sandbox_mode = FALSE;

/* Simple spin delay for playtime_seconds * 60 frames */
void credits_roll(const char *world_name, u32 playtime_seconds) {
    debug_log(LOG_INFO, "endgame: credits start");
    debug_log_int(LOG_INFO, "endgame: playtime_s", (s32)playtime_seconds);
    u32 frames = playtime_seconds * 60;
    u32 i;

    /* Suppress unused parameter warning */
    (void)world_name;

    /* Clear display */
    REG_DISPCNT = MODE3 | BG2_ENABLE;

    /* Fill screen with a dark color */
    {
        vu16 *vram = MEM_VRAM;
        u32 px;
        for (px = 0; px < 240 * 160; px++) {
            vram[px] = RGB15(2, 2, 6);
        }
    }

    /* Spin for the requested number of frames */
    for (i = 0; i < frames; i++) {
        vsync();
    }
}

void sandbox_mode_enable(void) {
    debug_log(LOG_INFO, "endgame: sandbox mode enabled");
    sandbox_mode = TRUE;
}

bool sandbox_mode_active(void) {
    return sandbox_mode;
}
