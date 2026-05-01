#include "crafting.h"

/* Task 5.1 — add ItemType enum values (ITEM_* constants)
   Task 5.2 — implement crafting_init (define recipe_book[]),
              crafting_unlock_check, crafting_execute */

Recipe recipe_book[MAX_RECIPES];
u8     recipe_count = 0;
