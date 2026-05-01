#pragma once
#include "../core/types.h"
#include "../world/section.h"

void init_renderer(void);
void load_bg_tiles(const u32 *tile_data, u16 tile_count);
void set_bg_tile(u8 x, u8 y, u8 tile_id);
void section_render_visible(const Section *s, s16 cam_x, s16 cam_y);
void render_structures(const Section *s);
