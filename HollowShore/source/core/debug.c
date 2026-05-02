#include "debug.h"

#define MGBA_REG_ENABLE  (*(vu16*)0x04FFF780)
#define MGBA_REG_FLAGS   (*(vu16*)0x04FFF700)
#define MGBA_BUF         ((volatile char*)0x04FFF600)
#define MGBA_BUF_LEN     256

#define MGBA_KEY_ENABLE  0xC0DE
#define MGBA_KEY_ACK     0x1DEA

static bool g_available = FALSE;

void debug_init(void) {
    MGBA_REG_ENABLE = MGBA_KEY_ENABLE;
    g_available = (MGBA_REG_ENABLE == MGBA_KEY_ACK) ? TRUE : FALSE;
    if (g_available)
        debug_log(LOG_INFO, "HollowShore: debug init OK");
}

bool debug_available(void) {
    return g_available;
}

void debug_log(u8 level, const char *msg) {
    if (!g_available) return;
    volatile char *dst = MGBA_BUF;
    u32 i = 0;
    while (*msg && i < MGBA_BUF_LEN - 1) {
        *dst++ = *msg++;
        i++;
    }
    *dst = '\0';
    MGBA_REG_FLAGS = ((u16)(level & 0x7)) | 0x100;
}

/* Write a signed integer as decimal into buf; returns chars written */
static u32 fmt_int(s32 val, char *buf) {
    u32 i = 0;
    if (val < 0) { buf[i++] = '-'; val = -val; }
    if (val == 0) { buf[i++] = '0'; buf[i] = '\0'; return i; }
    char tmp[12];
    u32 t = 0;
    while (val > 0) { tmp[t++] = '0' + (val % 10); val /= 10; }
    while (t > 0) buf[i++] = tmp[--t];
    buf[i] = '\0';
    return i;
}

void debug_log_int(u8 level, const char *label, s32 value) {
    if (!g_available) return;
    volatile char *dst = MGBA_BUF;
    u32 i = 0;

    /* write label */
    while (*label && i < MGBA_BUF_LEN - 16) { *dst++ = *label++; i++; }
    *dst++ = ':'; *dst++ = ' '; i += 2;

    /* write integer */
    char num[14];
    u32 nlen = fmt_int(value, num);
    u32 n = 0;
    while (n < nlen && i < MGBA_BUF_LEN - 1) { *dst++ = num[n++]; i++; }
    *dst = '\0';

    MGBA_REG_FLAGS = ((u16)(level & 0x7)) | 0x100;
}
