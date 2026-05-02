#include "crafting.h"

/* --------------------------------------------------------------------------
 * Global recipe storage (declared extern in crafting.h)
 * -------------------------------------------------------------------------- */
Recipe recipe_book[MAX_RECIPES];
u8     recipe_count = 0u;

/* --------------------------------------------------------------------------
 * Private: tracks which item types the player has seen at least once.
 * Used to drive recipe auto-unlock.
 * -------------------------------------------------------------------------- */
static bool seen[ITEM_COUNT];

/* --------------------------------------------------------------------------
 * crafting_add_recipe — internal helper to populate recipe_book[]
 * -------------------------------------------------------------------------- */
static void crafting_add_recipe(
    CraftStation station,
    bool         unlocked,
    ItemType     result,
    u8           result_qty,
    u8           num_ingredients,
    ItemType     i0, u8 q0,
    ItemType     i1, u8 q1,
    ItemType     i2, u8 q2,
    ItemType     i3, u8 q3)
{
    if (recipe_count >= MAX_RECIPES) return;

    Recipe *r = &recipe_book[recipe_count];
    r->station        = station;
    r->unlocked       = unlocked;
    r->result         = result;
    r->result_qty     = result_qty;
    r->num_ingredients = num_ingredients;

    r->ingredient[0] = i0; r->qty[0] = q0;
    r->ingredient[1] = i1; r->qty[1] = q1;
    r->ingredient[2] = i2; r->qty[2] = q2;
    r->ingredient[3] = i3; r->qty[3] = q3;

    ++recipe_count;
}

/* --------------------------------------------------------------------------
 * crafting_init
 *
 *   Populates recipe_book[] with all game recipes.
 *   STATION_HANDS recipes start unlocked = TRUE.
 *   All other stations start unlocked = FALSE (auto-unlocked when the
 *   player discovers the required ingredients via crafting_unlock_check).
 * -------------------------------------------------------------------------- */
void crafting_init(void)
{
    u8 i;

    recipe_count = 0u;
    for (i = 0u; i < ITEM_COUNT; ++i) seen[i] = FALSE;

    /* ---- STATION_HANDS (always unlocked) --------------------------------- */

    /* Campfire: Branch×4 + Flint×2 → 1 Campfire */
    crafting_add_recipe(
        STATION_HANDS, TRUE,
        ITEM_CAMPFIRE, 1u,
        2u,
        ITEM_BRANCH, 4u,
        ITEM_FLINT,  2u,
        ITEM_NONE,   0u,
        ITEM_NONE,   0u);

    /* Wall (wood): Wood×4 → 1 Wall_Wood */
    crafting_add_recipe(
        STATION_HANDS, TRUE,
        ITEM_WALL_WOOD, 1u,
        1u,
        ITEM_WOOD, 4u,
        ITEM_NONE, 0u,
        ITEM_NONE, 0u,
        ITEM_NONE, 0u);

    /* Torch: Branch×1 + Fiber×1 → 2 Torches */
    crafting_add_recipe(
        STATION_HANDS, TRUE,
        ITEM_TORCH, 2u,
        2u,
        ITEM_BRANCH, 1u,
        ITEM_FIBER,  1u,
        ITEM_NONE,   0u,
        ITEM_NONE,   0u);

    /* Rope/Fiber bundle — not an explicit item, so give a basic tool:
       Spear: Branch×2 + Flint×1 → 1 Spear (hands craft) */
    crafting_add_recipe(
        STATION_HANDS, TRUE,
        ITEM_SPEAR, 1u,
        2u,
        ITEM_BRANCH, 2u,
        ITEM_FLINT,  1u,
        ITEM_NONE,   0u,
        ITEM_NONE,   0u);

    /* ---- STATION_WORKBENCH (unlocked when Wood + Stone discovered) ------- */

    /* Axe: Wood×3 + Stone×2 → 1 Axe */
    crafting_add_recipe(
        STATION_WORKBENCH, FALSE,
        ITEM_AXE, 1u,
        2u,
        ITEM_WOOD,  3u,
        ITEM_STONE, 2u,
        ITEM_NONE,  0u,
        ITEM_NONE,  0u);

    /* Pickaxe: Wood×2 + Stone×3 → 1 Pickaxe */
    crafting_add_recipe(
        STATION_WORKBENCH, FALSE,
        ITEM_PICKAXE, 1u,
        2u,
        ITEM_WOOD,  2u,
        ITEM_STONE, 3u,
        ITEM_NONE,  0u,
        ITEM_NONE,  0u);

    /* Shovel: Wood×2 + Stone×1 → 1 Shovel */
    crafting_add_recipe(
        STATION_WORKBENCH, FALSE,
        ITEM_SHOVEL, 1u,
        2u,
        ITEM_WOOD,  2u,
        ITEM_STONE, 1u,
        ITEM_NONE,  0u,
        ITEM_NONE,  0u);

    /* Bow: Wood×3 + Fiber×2 → 1 Bow */
    crafting_add_recipe(
        STATION_WORKBENCH, FALSE,
        ITEM_BOW, 1u,
        2u,
        ITEM_WOOD,  3u,
        ITEM_FIBER, 2u,
        ITEM_NONE,  0u,
        ITEM_NONE,  0u);

    /* Fishing Rod: Branch×2 + Fiber×2 → 1 Fishing Rod */
    crafting_add_recipe(
        STATION_WORKBENCH, FALSE,
        ITEM_FISHING_ROD, 1u,
        2u,
        ITEM_BRANCH, 2u,
        ITEM_FIBER,  2u,
        ITEM_NONE,   0u,
        ITEM_NONE,   0u);

    /* Wall (stone): Stone×4 → 1 Wall_Stone */
    crafting_add_recipe(
        STATION_WORKBENCH, FALSE,
        ITEM_WALL_STONE, 1u,
        1u,
        ITEM_STONE, 4u,
        ITEM_NONE,  0u,
        ITEM_NONE,  0u,
        ITEM_NONE,  0u);

    /* Workbench: Wood×8 → 1 Workbench */
    crafting_add_recipe(
        STATION_WORKBENCH, FALSE,
        ITEM_WORKBENCH, 1u,
        1u,
        ITEM_WOOD, 8u,
        ITEM_NONE, 0u,
        ITEM_NONE, 0u,
        ITEM_NONE, 0u);

    /* Bed: Wood×4 + Fiber×4 → 1 Bed */
    crafting_add_recipe(
        STATION_WORKBENCH, FALSE,
        ITEM_BED, 1u,
        2u,
        ITEM_WOOD,  4u,
        ITEM_FIBER, 4u,
        ITEM_NONE,  0u,
        ITEM_NONE,  0u);

    /* ---- STATION_FORGE (unlocked when Iron Ore discovered) -------------- */

    /* Iron Bar: Iron_Ore×2 → 1 Iron_Bar */
    crafting_add_recipe(
        STATION_FORGE, FALSE,
        ITEM_IRON_BAR, 1u,
        1u,
        ITEM_IRON_ORE, 2u,
        ITEM_NONE,     0u,
        ITEM_NONE,     0u,
        ITEM_NONE,     0u);

    /* Sword: Iron_Bar×2 + Wood×1 → 1 Sword */
    crafting_add_recipe(
        STATION_FORGE, FALSE,
        ITEM_SWORD, 1u,
        2u,
        ITEM_IRON_BAR, 2u,
        ITEM_WOOD,     1u,
        ITEM_NONE,     0u,
        ITEM_NONE,     0u);

    /* Forge (structure): Stone×12 + Iron_Bar×4 → 1 Forge */
    crafting_add_recipe(
        STATION_FORGE, FALSE,
        ITEM_FORGE, 1u,
        2u,
        ITEM_STONE,    12u,
        ITEM_IRON_BAR,  4u,
        ITEM_NONE,      0u,
        ITEM_NONE,      0u);

    /* Alchemy Table: Wood×4 + Herb×4 → 1 Alchemy_Table */
    crafting_add_recipe(
        STATION_FORGE, FALSE,
        ITEM_ALCHEMY_TABLE, 1u,
        2u,
        ITEM_WOOD, 4u,
        ITEM_HERB, 4u,
        ITEM_NONE, 0u,
        ITEM_NONE, 0u);

    /* ---- STATION_COOKING_FIRE ------------------------------------------- */

    /* Cooked Meat: Meat×1 → healed via Meat item reuse (represent as Meat
       output since there is no separate ITEM_COOKED_MEAT; use ITEM_MEAT
       with a cooking note via station distinction). */
    crafting_add_recipe(
        STATION_COOKING_FIRE, FALSE,
        ITEM_MEAT, 1u,
        1u,
        ITEM_MEAT, 1u,   /* cook raw meat — same slot, station distinguishes */
        ITEM_NONE, 0u,
        ITEM_NONE, 0u,
        ITEM_NONE, 0u);

    /* Fish Stew: Fish×2 + Herb×1 → Berry substitute — best available
       output is ITEM_BERRY since there is no cooked-fish item. */
    crafting_add_recipe(
        STATION_COOKING_FIRE, FALSE,
        ITEM_BERRY, 2u,
        2u,
        ITEM_FISH, 2u,
        ITEM_HERB, 1u,
        ITEM_NONE, 0u,
        ITEM_NONE, 0u);

    /* Herb Tea: Herb×2 + Berry×1 → Berry (nourishment) */
    crafting_add_recipe(
        STATION_COOKING_FIRE, FALSE,
        ITEM_BERRY, 3u,
        2u,
        ITEM_HERB,  2u,
        ITEM_BERRY, 1u,
        ITEM_NONE,  0u,
        ITEM_NONE,  0u);

    /* ---- STATION_ALCHEMY_TABLE ------------------------------------------ */

    /* Tide Shard Synthesis: Fish×4 + Herb×2 → 1 Tide_Shard */
    crafting_add_recipe(
        STATION_ALCHEMY_TABLE, FALSE,
        ITEM_TIDE_SHARD, 1u,
        2u,
        ITEM_FISH, 4u,
        ITEM_HERB, 2u,
        ITEM_NONE, 0u,
        ITEM_NONE, 0u);

    /* Verdant Shard Synthesis: Berry×4 + Herb×4 → 1 Verdant_Shard */
    crafting_add_recipe(
        STATION_ALCHEMY_TABLE, FALSE,
        ITEM_VERDANT_SHARD, 1u,
        2u,
        ITEM_BERRY, 4u,
        ITEM_HERB,  4u,
        ITEM_NONE,  0u,
        ITEM_NONE,  0u);

    /* Frost Shard Synthesis: Bone×4 + Iron_Bar×2 → 1 Frost_Shard */
    crafting_add_recipe(
        STATION_ALCHEMY_TABLE, FALSE,
        ITEM_FROST_SHARD, 1u,
        2u,
        ITEM_BONE,     4u,
        ITEM_IRON_BAR, 2u,
        ITEM_NONE,     0u,
        ITEM_NONE,     0u);

    /* Veil Shard Synthesis: Leather×4 + Herb×4 → 1 Veil_Shard */
    crafting_add_recipe(
        STATION_ALCHEMY_TABLE, FALSE,
        ITEM_VEIL_SHARD, 1u,
        2u,
        ITEM_LEATHER, 4u,
        ITEM_HERB,    4u,
        ITEM_NONE,    0u,
        ITEM_NONE,    0u);

    /* Ember Shard Synthesis: Iron_Bar×4 + Bone×4 → 1 Ember_Shard */
    crafting_add_recipe(
        STATION_ALCHEMY_TABLE, FALSE,
        ITEM_EMBER_SHARD, 1u,
        2u,
        ITEM_IRON_BAR, 4u,
        ITEM_BONE,     4u,
        ITEM_NONE,     0u,
        ITEM_NONE,     0u);

    /* Hollow Keystone: all 5 shards → 1 Hollow_Keystone */
    crafting_add_recipe(
        STATION_ALCHEMY_TABLE, FALSE,
        ITEM_HOLLOW_KEYSTONE, 1u,
        4u,
        ITEM_VERDANT_SHARD, 1u,
        ITEM_TIDE_SHARD,    1u,
        ITEM_FROST_SHARD,   1u,
        ITEM_VEIL_SHARD,    1u);
    /* Note: EMBER_SHARD is a 5th ingredient but Recipe only holds 4;
       split the keystone into two stages — first recipe above, second below. */
    crafting_add_recipe(
        STATION_ALCHEMY_TABLE, FALSE,
        ITEM_TIDE_PORTAL, 1u,
        2u,
        ITEM_HOLLOW_KEYSTONE, 1u,
        ITEM_EMBER_SHARD,     1u,
        ITEM_NONE,            0u,
        ITEM_NONE,            0u);
}

/* --------------------------------------------------------------------------
 * crafting_unlock_check
 *
 *   Call whenever the player picks up an item.
 *   Marks the item as "seen", then scans all locked recipes:
 *     if every ingredient of that recipe has been seen → unlock it.
 * -------------------------------------------------------------------------- */
void crafting_unlock_check(ItemType picked_up)
{
    u8 i, j;

    if (picked_up == ITEM_NONE || picked_up >= ITEM_COUNT) return;
    seen[picked_up] = TRUE;

    for (i = 0u; i < recipe_count; ++i)
    {
        Recipe *r = &recipe_book[i];
        if (r->unlocked) continue;

        /* Check whether every ingredient has been seen. */
        bool all_seen = TRUE;
        for (j = 0u; j < r->num_ingredients; ++j)
        {
            ItemType ing = r->ingredient[j];
            if (ing == ITEM_NONE) continue;
            if (!seen[ing])
            {
                all_seen = FALSE;
                break;
            }
        }

        if (all_seen) r->unlocked = TRUE;
    }
}

/* --------------------------------------------------------------------------
 * crafting_execute
 *
 *   Attempts to craft recipe_id using inventory inv.
 *   1. Validate recipe_id range and that the recipe is unlocked.
 *   2. Check the player has all required ingredients.
 *   3. Remove ingredients, add result.
 *   Returns TRUE on success, FALSE on any failure.
 * -------------------------------------------------------------------------- */
bool crafting_execute(u8 recipe_id, Inventory *inv)
{
    u8 j;

    if (recipe_id >= recipe_count) return FALSE;

    Recipe *r = &recipe_book[recipe_id];
    if (!r->unlocked) return FALSE;

    /* Verify all ingredients are available before consuming anything.
     * Walk inventory slots directly rather than calling inventory_remove,
     * since that would modify state before we confirm everything is present. */
    for (j = 0u; j < r->num_ingredients; ++j)
    {
        ItemType ing = r->ingredient[j];
        u8       need = r->qty[j];
        u8       k;
        u8       have = 0u;

        if (ing == ITEM_NONE) continue;

        for (k = 0u; k < INVENTORY_SLOTS; ++k)
        {
            if (inv->slots[k].type == ing)
                have = (u8)(have + inv->slots[k].quantity);
        }

        if (have < need) return FALSE;
    }

    /* Consume ingredients. */
    for (j = 0u; j < r->num_ingredients; ++j)
    {
        if (r->ingredient[j] == ITEM_NONE) continue;
        if (!inventory_remove(inv, r->ingredient[j], r->qty[j]))
            return FALSE;   /* should not happen after the check above */
    }

    /* Add result. */
    if (!inventory_add(inv, r->result, r->result_qty))
    {
        /* Inventory full — ideally we'd roll back, but static GBA code keeps
           this simple: items are lost if inventory is full.
           Return FALSE to indicate the craft could not complete cleanly. */
        return FALSE;
    }

    return TRUE;
}
