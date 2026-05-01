#pragma once
#include "../core/types.h"

#define MAX_SPRITES  128

void oam_init(void);
void sprite_set(u8 id, s16 x, s16 y, u8 tile, u8 pal);
void sprite_hide(u8 id);
void copy_oam(void);  /* DMA transfer OAM buffer → OAM */
