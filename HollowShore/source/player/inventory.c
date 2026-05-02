#include "inventory.h"

/* ---- inventory_init ------------------------------------------------------ */
void inventory_init(Inventory *inv)
{
    u8 i;
    for (i = 0; i < INVENTORY_SLOTS; i++)
    {
        inv->slots[i].type     = ITEM_NONE;
        inv->slots[i].quantity = 0;
    }
    inv->hotbar_cursor = 0;
}

/* ---- inventory_add ------------------------------------------------------- */
/*
 * First try to stack onto an existing slot of the same type (max 99).
 * Then find an empty slot.  Returns FALSE if full.
 * Scans all 24 slots (main 0-15 and hotbar 16-23) uniformly.
 */
bool inventory_add(Inventory *inv, ItemType type, u8 qty)
{
    u8 i;
    u8 remaining = qty;

    if (type == ITEM_NONE || qty == 0)
        return FALSE;

    /* Pass 1: stack onto existing slots */
    for (i = 0; i < INVENTORY_SLOTS && remaining > 0; i++)
    {
        if (inv->slots[i].type == type && inv->slots[i].quantity < 99)
        {
            u8 space = (u8)(99 - inv->slots[i].quantity);
            if (space >= remaining)
            {
                inv->slots[i].quantity = (u8)(inv->slots[i].quantity + remaining);
                remaining = 0;
            }
            else
            {
                inv->slots[i].quantity = 99;
                remaining = (u8)(remaining - space);
            }
        }
    }

    /* Pass 2: fill empty slots */
    for (i = 0; i < INVENTORY_SLOTS && remaining > 0; i++)
    {
        if (inv->slots[i].type == ITEM_NONE)
        {
            inv->slots[i].type = type;
            if (remaining > 99)
            {
                inv->slots[i].quantity = 99;
                remaining = (u8)(remaining - 99);
            }
            else
            {
                inv->slots[i].quantity = remaining;
                remaining = 0;
            }
        }
    }

    return (remaining == 0) ? TRUE : FALSE;
}

/* ---- inventory_remove ---------------------------------------------------- */
/*
 * Scans all slots for the requested type, decrements quantity.
 * Sets slot to ITEM_NONE when quantity reaches 0.
 * Returns FALSE if the type is not found or total quantity is insufficient.
 */
bool inventory_remove(Inventory *inv, ItemType type, u8 qty)
{
    u8 i;
    u8 total = 0;
    u8 need  = qty;

    if (type == ITEM_NONE || qty == 0)
        return FALSE;

    /* Count total available */
    for (i = 0; i < INVENTORY_SLOTS; i++)
    {
        if (inv->slots[i].type == type)
            total = (u8)(total + inv->slots[i].quantity);
    }

    if (total < need)
        return FALSE;

    /* Deduct */
    for (i = 0; i < INVENTORY_SLOTS && need > 0; i++)
    {
        if (inv->slots[i].type == type)
        {
            if (inv->slots[i].quantity <= need)
            {
                need = (u8)(need - inv->slots[i].quantity);
                inv->slots[i].quantity = 0;
                inv->slots[i].type     = ITEM_NONE;
            }
            else
            {
                inv->slots[i].quantity = (u8)(inv->slots[i].quantity - need);
                need = 0;
            }
        }
    }

    return TRUE;
}

/* ---- inventory_sort ------------------------------------------------------ */
/*
 * Insertion sort on main slots (0–15) by ItemType, ascending.
 * Hotbar slots (16–23) are left untouched per spec.
 */
void inventory_sort(Inventory *inv)
{
    u8   i, j;
    Item key;

#define MAIN_SLOTS 16

    for (i = 1; i < MAIN_SLOTS; i++)
    {
        key = inv->slots[i];
        j   = i;
        while (j > 0 && inv->slots[j - 1].type > key.type)
        {
            inv->slots[j] = inv->slots[j - 1];
            j--;
        }
        inv->slots[j] = key;
    }

#undef MAIN_SLOTS
}

/* ---- inventory_hotbar_active --------------------------------------------- */
/*
 * Hotbar occupies slots 16–23.
 * hotbar_cursor is 0–7.
 */
Item *inventory_hotbar_active(Inventory *inv)
{
    return &inv->slots[16 + inv->hotbar_cursor];
}
