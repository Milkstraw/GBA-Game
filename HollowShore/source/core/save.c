#include "save.h"
#include "debug.h"
#include "../gba.h"

#define SRAM_BASE ((vu8*)0x0E000000)

/* Compute checksum: sum of all bytes except the checksum field itself */
static u16 compute_checksum(const SaveData *data) {
    const u8 *p = (const u8*)data;
    u32 sum = 0;
    u32 i;
    u32 size = sizeof(SaveData) - sizeof(u16); /* exclude checksum at end */
    for (i = 0; i < size; i++) {
        sum += p[i];
    }
    return (u16)(sum & 0xFFFF);
}

void save_write(const SaveData *data) {
    debug_log(LOG_INFO, "save: writing");
    const u8 *src = (const u8*)data;
    u32 i;
    u32 size = sizeof(SaveData) - sizeof(u16);

    /* Write all fields except checksum */
    for (i = 0; i < size; i++) {
        SRAM_BASE[i] = src[i];
    }

    /* Write computed checksum */
    u16 checksum = compute_checksum(data);
    SRAM_BASE[size]     = (u8)(checksum & 0xFF);
    SRAM_BASE[size + 1] = (u8)(checksum >> 8);
}

void save_read(SaveData *data) {
    debug_log(LOG_INFO, "save: reading");
    u8 *dst = (u8*)data;
    u32 i;
    u32 size = sizeof(SaveData);
    for (i = 0; i < size; i++) {
        dst[i] = SRAM_BASE[i];
    }
}

bool save_validate_checksum(const SaveData *data) {
    u16 computed = compute_checksum(data);
    bool ok = (computed == data->checksum) ? TRUE : FALSE;
    if (ok)
        debug_log(LOG_INFO, "save: checksum OK");
    else
        debug_log(LOG_ERROR, "save: checksum FAIL — corrupt or no save");
    return ok;
}
