#include "bosses.h"

/* Shared boss state. Each sanctum .c file drives its own boss update loop. */

Boss active_boss;
