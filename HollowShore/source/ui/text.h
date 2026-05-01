#pragma once
#include "../core/types.h"

/* 8×8 pixel font rendered to BG3 tile layer. */
void text_init(void);
void text_draw(u8 x, u8 y, const char *str);
void text_draw_int(u8 x, u8 y, s32 val);
void text_clear(void);
