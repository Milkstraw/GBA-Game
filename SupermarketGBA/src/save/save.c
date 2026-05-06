#include "save.h"
#include "inventory.h"
#include "daytimer.h"
#include "constants.h"

/*
 * SRAM byte access helpers.
 * GBA SRAM is on an 8-bit bus; 16/32-bit writes corrupt neighbouring bytes.
 * All SRAM I/O must go through sram_read_byte / sram_write_byte.
 */
static u8 sram_read_byte(u16 offset)
{
    return SRAM_BASE[offset];
}

static void sram_write_byte(u16 offset, u8 val)
{
    SRAM_BASE[offset] = val;
}

/* ----------------------------------------------------------------
 * Serialise SaveSlot → SRAM at slot_offset, return final checksum.
 * Writes every byte individually; computes XOR checksum on the fly.
 * ---------------------------------------------------------------- */
static u8 write_slot(u16 base, const SaveSlot *s)
{
    const u8 *p   = (const u8 *)s;
    u16        sz  = (u16)(sizeof(SaveSlot) - 1u); /* all bytes except checksum */
    u8         chk = 0;
    u16        i;

    for (i = 0; i < sz; i++) {
        chk ^= p[i];
        sram_write_byte((u16)(base + i), p[i]);
    }
    /* Write checksum as last byte */
    sram_write_byte((u16)(base + sz), chk);
    return chk;
}

/* ----------------------------------------------------------------
 * Read SRAM at slot_offset into SaveSlot; return computed checksum.
 * ---------------------------------------------------------------- */
static u8 read_slot(u16 base, SaveSlot *s)
{
    u8  *p   = (u8 *)s;
    u16  sz  = (u16)(sizeof(SaveSlot) - 1u);
    u8   chk = 0;
    u16  i;

    for (i = 0; i < sz; i++) {
        p[i] = sram_read_byte((u16)(base + i));
        chk ^= p[i];
    }
    /* Read stored checksum */
    s->checksum = sram_read_byte((u16)(base + sz));
    return chk;
}

/* ----------------------------------------------------------------
 * save_write
 * ---------------------------------------------------------------- */
void save_write(u8 slot)
{
    SaveSlot sv;
    u16      base;
    u8       i;

    if (slot >= SAVE_NUM_SLOTS) return;
    base = (u16)(slot * SAVE_SLOT_SIZE);

    sv.day_number    = g_daytimer.day;
    sv.cash_balance  = (u32)g_cash;
    sv.store_level   = 1;
    sv.reputation    = 0;
    sv.total_revenue = 0;  /* Phase 2+ tracks this */

    sv.store_name[0] = 'M';
    sv.store_name[1] = 'Y';
    sv.store_name[2] = ' ';
    sv.store_name[3] = 'S';
    sv.store_name[4] = 'T';
    sv.store_name[5] = 'O';
    sv.store_name[6] = 'R';
    sv.store_name[7] = 'E';
    for (i = 8; i < 17u; i++) sv.store_name[i] = '\0';

    for (i = 0; i < MAX_PRODUCTS; i++) {
        sv.products[i].on_shelf   = g_products[i].stock_on_shelf;
        sv.products[i].in_back    = g_products[i].stock_in_back;
        sv.products[i].sold_total = (u16)g_products[i].units_sold_today;
    }

    sv.checksum = 0;  /* placeholder; write_slot computes real value */
    write_slot(base, &sv);
}

/* ----------------------------------------------------------------
 * save_read
 * ---------------------------------------------------------------- */
u8 save_read(u8 slot)
{
    SaveSlot sv;
    u16      base;
    u8       computed_chk;
    u8       i;

    if (slot >= SAVE_NUM_SLOTS) return 0;
    base = (u16)(slot * SAVE_SLOT_SIZE);

    computed_chk = read_slot(base, &sv);

    /* Verify checksum — do NOT restore state on mismatch */
    if (computed_chk != sv.checksum) return 0;

    /* Restore game state */
    g_cash              = (u16)sv.cash_balance;
    g_daytimer.day      = (u8)sv.day_number;

    for (i = 0; i < MAX_PRODUCTS; i++) {
        g_products[i].stock_on_shelf = sv.products[i].on_shelf;
        g_products[i].stock_in_back  = sv.products[i].in_back;
        /* Recalculate backroom total from deserialized data */
    }

    /* Recompute g_backroom_total from product data */
    g_backroom_total = 0;
    for (i = 0; i < MAX_PRODUCTS; i++) {
        g_backroom_total = (u8)(g_backroom_total + g_products[i].stock_in_back);
    }

    return 1;
}

/* ----------------------------------------------------------------
 * save_slot_exists
 * ---------------------------------------------------------------- */
u8 save_slot_exists(u8 slot)
{
    SaveSlot sv;
    u16      base;
    u8       computed_chk;

    if (slot >= SAVE_NUM_SLOTS) return 0;
    base = (u16)(slot * SAVE_SLOT_SIZE);

    computed_chk = read_slot(base, &sv);
    return (computed_chk == sv.checksum) ? 1u : 0u;
}

/* ----------------------------------------------------------------
 * Override daytimer_end_of_day weak stub: save before resetting stats.
 * ---------------------------------------------------------------- */
void daytimer_end_of_day(void)
{
    u8 i;

    /* Save before resetting daily stats */
    save_write(0);

    /* Reset daily sales */
    for (i = 0; i < MAX_PRODUCTS; i++) {
        g_products[i].units_sold_today = 0;
    }

    g_daytimer.day++;
    g_daytimer.minute         = 0;
    g_daytimer.hour           = 8;
    g_daytimer.minute_of_hour = 0;
    g_daytimer.frame_accum    = 0;
    g_daytimer.store_open     = 1;
}
