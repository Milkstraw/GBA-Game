#pragma once
#include "types.h"

typedef struct {
    u8  section_id;
    s16 player_x;
    s16 player_y;
    u16 day;
    u8  hour;
    u8  season;
    u8  weather;
    u8  boss_flags;     /* bitmask, one bit per sanctum */
    /* inventory, tile-change bitfields, etc. added in Task 9.1 */
    u16 checksum;
} SaveData;

void save_write(const SaveData *data);
void save_read(SaveData *data);
bool save_validate_checksum(const SaveData *data);
