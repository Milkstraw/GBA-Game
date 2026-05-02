#pragma once
#include "types.h"

/* mGBA emulator debug output — silent on real hardware */

#define LOG_FATAL  0
#define LOG_ERROR  1
#define LOG_WARN   2
#define LOG_INFO   3
#define LOG_DEBUG  4

void debug_init(void);
bool debug_available(void);

void debug_log(u8 level, const char *msg);
void debug_log_int(u8 level, const char *label, s32 value);
