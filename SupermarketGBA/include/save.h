#ifndef SAVE_H
#define SAVE_H

#include "gba_types.h"
#include "constants.h"

/*
 * GBA SRAM: 32KB at 0x0E000000.
 * 8-bit bus — ONLY byte-wide reads/writes are valid.
 * Layout: 3 slots × 10240 bytes = 30720 bytes (under 32KB limit).
 */
#define SAVE_SLOT_SIZE    10240u   /* bytes per slot */
#define SAVE_NUM_SLOTS    3u

/* Per-product save record (4 bytes) */
typedef struct {
    u8  on_shelf;
    u8  in_back;
    u16 sold_total;    /* accumulated across days */
} ProductSave;

/*
 * SaveSlot layout (packed into ≤ 10240 bytes):
 *   2  day_number
 *   4  cash_balance (cents, u32 for future use; written/read as 4 bytes)
 *   1  store_level
 *   1  reputation
 *   4  total_revenue (cents)
 *  17  store_name (16 chars + null)
 * 128  product_save[32] (4 bytes each)
 *   1  checksum (XOR of all preceding bytes in slot)
 * -------
 * 158  bytes total — well within 10240
 */
typedef struct {
    u16        day_number;
    u32        cash_balance;
    u8         store_level;
    u8         reputation;
    u32        total_revenue;
    char       store_name[17];
    ProductSave products[MAX_PRODUCTS];
    u8         checksum;
} SaveSlot;

/* ----------------------------------------------------------------
 * API
 * ---------------------------------------------------------------- */

/*
 * Serialise current game state into SaveSlot and write to SRAM slot 0-2.
 * SRAM is accessed one byte at a time (8-bit bus).
 */
void save_write(u8 slot);

/*
 * Read SaveSlot from SRAM, verify checksum.
 * On success: restores g_cash, product stocks, day number.
 * Returns 1 if checksum valid, 0 if corrupt (does not modify live state).
 */
u8 save_read(u8 slot);

/*
 * Returns 1 if a valid (checksum-passing) save exists in slot, 0 otherwise.
 * Does not modify live state.
 */
u8 save_slot_exists(u8 slot);

#endif /* SAVE_H */
