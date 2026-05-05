# Supermarket GBA — AI Agent Build Prompts

## How To Use This Document

Each prompt is a self-contained task for one agent. Run agents in order within each phase. After each agent completes, run the matching REVIEW prompt before moving to the next task. Coordinator prompt goes first in every phase to orient the agent on what exists before it writes anything.

**Stack:** C, devkitARM, libtonc, GBA Mode 0  
**No floats. No dynamic alloc. All prices in cents. All arrays fixed size.**  
**GDD reference:** `supermarket_gba_gdd.md`

-----

## PHASE 1 — DEMO

-----

### P1-00 · COORDINATOR

```
You coordinate GBA game build. Stack: C, devkitARM, libtonc, GBA Mode 0.
No floats. No malloc. All prices stored as uint16 cents. Fixed arrays only.
Project has no files yet. Your job: output a file tree showing every .c and .h
file needed for Phase 1 demo. No code. Just the tree with one-line description
per file. Include Makefile. Group by: core/, gfx/, map/, player/, npc/,
ui/, save/. After tree, list build order — which files must exist before others.
```

-----

### P1-01 · MAKEFILE + PROJECT SCAFFOLD

```
GBA project. devkitARM toolchain. libtonc.
Write Makefile that: finds all .c in src/ recursively, compiles to .o in
build/, links to .elf, converts to .gba, runs gbafix. Flags: -O2 -mthumb
-mthumb-interwork -nostartfiles. Include clean target. Include a main.c in
src/ with empty main() that inits libtonc and loops forever. Project must
compile to a valid blank GBA rom before any game code is added.
```

**REVIEW P1-01**

```
Review this Makefile and main.c for a GBA devkitARM project.
Check: correct ARM flags, libtonc linked, gbafix called, .gba output produced,
clean target works, compiles without error. List any bugs. Fix them. Output
corrected files only.
```

-----

### P1-02 · MEMORY MAP + CONSTANTS HEADER

```
GBA C project. Write include/gba_types.h and include/constants.h. No code,
headers only.

gba_types.h: typedef u8/u16/u32/s8/s16/s32. Define REG_DISPCNT, REG_BG0CNT,
REG_BG1CNT, REG_BG2CNT, REG_BG3CNT, REG_BG0HOFS, REG_BG0VOFS and matching
BG1-3. Define OAM base address. Define VRAM, PALRAM, OAM base pointers as
volatile pointers. Define KEYS register and all key bit masks (A,B,UP,DOWN,
LEFT,RIGHT,START,SELECT).

constants.h: TILE_W=16, TILE_H=16, MAP_W=15, MAP_H=10, SCREEN_W=240,
SCREEN_H=160, HUD_H=10. SRAM_BASE address. MAX_NPCS=8, MAX_SHELVES=32,
MAX_PRODUCTS=32, MAX_ORDERS=8, BACKROOM_CAP=30. All prices as uint16 cents.
```

**REVIEW P1-02**

```
Review gba_types.h and constants.h for a GBA C project using devkitARM and
libtonc. Check: all MMIO addresses correct for GBA hardware, volatile on all
hardware registers, no floats, no malloc, constants match GDD spec
(TILE=16, MAP=15x10, SCREEN=240x160, HUD=10px, MAX_NPCS=8, BACKROOM_CAP=30,
MAX_PRODUCTS=32). Fix any wrong addresses or missing defines. Output corrected
headers only.
```

-----

### P1-03 · TILE ENGINE — BG INIT + MAP LOAD

```
GBA C. libtonc. Write src/gfx/tile_engine.c and include/tile_engine.h.

Mode 0. 4 BG layers. BG0=HUD fixed no scroll. BG1=foreground scrolls.
BG2=floor+walls scrolls. BG3=background deco scrolls.

tile_engine_init(): set REG_DISPCNT for mode 0, enable all 4 BGs, set
each BGxCNT for 256x256 map, 16-color tiles, correct charblock and
mapblock assignments (BG0 charblock 0, BG1 charblock 1, BG2 charblock 2,
BG3 charblock 3).

tile_engine_load_map(const u8* map_data, u16 w, u16 h, u8 bg_layer): writes
map_data tile IDs into the correct BG screenblock in VRAM. map_data is a
flat u8 array, row-major, w*h entries.

tile_engine_set_scroll(u8 bg, s16 dx, s16 dy): writes dx/dy to BGxHOFS/VOFS.

Expose: u8 g_collision_flags[256] — array indexed by tile ID, each byte is
the collision bitmask from GDD (bit0=solid, bit1=interact, bit2=shelf,
bit3=register, bit4=spawn, bit5=exit, bit6=backroom).

No graphics data yet — just the engine. Map data and tile graphics loaded
separately.
```

**REVIEW P1-03**

```
Review tile_engine.c and tile_engine.h for GBA Mode 0 using libtonc.
Check: correct charblock/screenblock assignments no overlap, VRAM writes use
correct volatile u16 pointer arithmetic, BG0 scroll locked (no HOFS/VOFS
writes for BG0), collision flag array is u8[256], map load writes correct
screenblock offsets for given bg_layer, no floats, no malloc. Fix bugs.
Output corrected files only.
```

-----

### P1-04 · DEMO MAP DATA

```
GBA C. Write src/map/demo_map.c and include/demo_map.h.

Map is 15 wide x 10 tall = 150 tiles. Use this layout exactly:

Row 0:  WC WT WT WT WT WT WT WT WT WT WT WT WT WT WC
Row 1:  WL PO PO SH SH SH FL FL SH SH SH PO PO PO WR
Row 2:  WL FL FL S1 S1 S1 FL FL S2 S2 S2 FL FL FL WR
Row 3:  WL FL FL FL FL FL FL FL FL FL FL FL FL FL WR
Row 4:  WL FL FL SH SH SH FL FL FL FL FL FL FL FL WR
Row 5:  WL FL FL S3 S3 S3 FL FL FL FL FL FL FL FL WR
Row 6:  WL FL FL FL FL FL FL FL FL FL FL FL FL FL WR
Row 7:  WL FL FL FL FL FL FL CT CT CT CT CT FL FL WR
Row 8:  WL FL FL FL FL FL FL RG FL FL FL FL FL FL WR
Row 9:  WC FL FL FL FL FL DM DR DR DM FL FL FL FL WC

Tile ID assignments:
WC=16, WT=17, WL=18, WR=19, FL=0, PO=96, SH=32,
S1=40(stocked)/32(empty), S2=40(stocked)/32(empty),
S3=32(always empty in demo), CT=48, RG=49, DM=64, DR=65.

Collision flags per tile ID:
FL(0): 0x00. SH(32): 0x01. S1/S2 stocked(40): 0x07. S3(32): 0x01.
CT(48): 0x01. RG(49): 0x09. DR(65): 0x30. DM(64): 0x00.
WC/WT/WL/WR(16-19): 0x01. PO(96): 0x01.

Export: const u8 demo_map_bg2[150] for floor+walls layer.
Export: const u8 demo_map_bg1[150] for shelf fronts and counter layer.
Export: void demo_map_load() that calls tile_engine_load_map for both layers
and populates g_collision_flags[].
Export: SHELF1_TILES[] = {(2,3),(3,3),(4,3)} — tile coords player faces to
interact with shelf 1. Same for SHELF2, SHELF3, REGISTER at (7,8).
```

**REVIEW P1-04**

```
Review demo_map.c and demo_map.h for a GBA C project.
Check: map array is exactly 150 u8 values, tile IDs match GDD spec, collision
flags match GDD bitmask (bit0=solid, bit1=interact, bit2=shelf, bit3=register,
bit4=spawn, bit5=exit), shelf interact tiles are the row IN FRONT of shelf not
the shelf tile itself (player stands at row 3 to interact with row 2 shelves),
register interact at (7,8), spawn/exit flags on DR tiles at (7,9) and (8,9).
Fix any mismatches. Output corrected files only.
```

-----

### P1-05 · INPUT SYSTEM

```
GBA C. Write src/core/input.c and include/input.h.

Read REG_KEYINPUT each frame. GBA keys are active-low — invert to get pressed.
Track: keys_held (held this frame), keys_pressed (newly down this frame),
keys_released (newly up this frame). Update via input_poll() called once
per frame.

Macros: KEY_PRESSED(k), KEY_HELD(k), KEY_RELEASED(k) using the three state
variables. Key masks from constants.h.

No debounce needed. No repeat. Just raw per-frame state.
```

**REVIEW P1-05**

```
Review input.c and input.h for GBA C.
Check: REG_KEYINPUT read as volatile u16, active-low correctly inverted,
keys_pressed = (cur & ~prev), keys_released = (~cur & prev), keys_held = cur,
all three updated each input_poll() call, macros use bitwise AND not equality.
Fix bugs. Output corrected files only.
```

-----

### P1-06 · PLAYER SYSTEM

```
GBA C. libtonc OAM. Write src/player/player.c and include/player.h.

Player sprite: 16x24px, 2 OAM entries (one 16x16, one 16x8 below it).
SPR PAL 0. Uses tile IDs from sprite sheet loaded separately.

Player struct:
  u8 x, y        — tile position
  u8 px, py      — pixel position (x*16, y*16, updated each move)
  u8 dir          — 0=down 1=up 2=left 3=right
  u8 anim_frame   — 0,1,2 cycling walk frames
  u8 anim_timer   — counts frames, advances anim_frame every 8 frames
  u8 is_moving    — flag

player_init(): place player at tile (6,6). Set OAM entries 0 and 1.

player_update(): read input_poll result. If dpad pressed: check collision flag
bit0 at target tile via g_collision_flags[map_tile_at(nx,ny)]. If passable,
move. Update px/py. Update dir. Cycle anim_frame. If A pressed: check tile in
front of player for bit1 (interact). If interact tile found, call
player_on_interact(tile_x, tile_y).

player_draw(): write px, py to OAM entries 0 and 1 with correct tile offset
for dir and anim_frame.

player_on_interact() is a weak stub — other systems hook it.
```

**REVIEW P1-06**

```
Review player.c and player.h for GBA C using libtonc OAM.
Check: tile collision check uses g_collision_flags bit0 before moving,
interact check looks at tile in FRONT of player based on dir (dir=down checks
y+1, dir=up checks y-1, dir=left checks x-1, dir=right checks x+1),
OAM entries 0 and 1 both updated each draw, pixel pos = tile pos * 16,
anim_frame cycles 0-1-2 on timer not on every frame, no floats, no malloc.
Fix bugs. Output corrected files only.
```

-----

### P1-07 · PRODUCT + INVENTORY DATA

```
GBA C. Write src/core/inventory.c and include/inventory.h.

Product struct (from GDD):
  u8  id
  char name[13]
  u8  category
  u16 shelf_price   (cents)
  u16 cost_price    (cents)
  u16 market_price  (cents)
  u8  stock_on_shelf
  u8  shelf_capacity
  u8  stock_in_back
  u8  units_sold_today

Global: Product g_products[MAX_PRODUCTS] in EWRAM (__attribute__((section(".ewram")))).
Global: u16 g_cash (cents). Start at 5000 (= $50.00).
Global: u8 g_backroom_total.

inventory_init(): populate g_products[0..2] with demo data:
  0: "Bread", cost=80, shelf_price=149, capacity=6, market=149
  1: "Milk",  cost=110, shelf_price=229, capacity=4, market=229
  2: "Apples",cost=50,  shelf_price=99,  capacity=8, market=99
  All start with stock_on_shelf=0, stock_in_back=0.

inventory_order(u8 product_id, u8 qty): if g_cash >= cost*qty, deduct cash,
add qty to stock_in_back, add qty to g_backroom_total. Return 1 on success,
0 on fail.

inventory_stock_shelf(u8 product_id): if stock_in_back>0, move 1 unit from
back to shelf (if shelf not full). Update g_backroom_total. Swap tile ID in
map if shelf was empty and now has stock (call demo_map_set_shelf_state).

inventory_sell(u8 product_id): if stock_on_shelf>0, decrement stock_on_shelf,
add shelf_price to g_cash, increment units_sold_today. Swap tile if now empty.
Return 1 on success, 0 on fail.
```

**REVIEW P1-07**

```
Review inventory.c and inventory.h for GBA C.
Check: Product array in EWRAM section, g_cash starts at 5000 cents,
inventory_order deducts cost*qty not just cost, g_backroom_total updated on
both order and stock_shelf, stock_shelf respects shelf_capacity, sell
decrements on_shelf not in_back, units_sold_today incremented on sell,
tile swap called when shelf transitions empty<->stocked, no floats, no malloc.
Fix bugs. Output corrected files only.
```

-----

### P1-08 · DAY TIMER SYSTEM

```
GBA C. Write src/core/daytimer.c and include/daytimer.h.

GBA runs at 59.727 fps. 1 in-game minute = 4 real frames (so 1 in-game hour
= 240 frames, 1 in-game day ~= 2880 frames for 12hr store day).

Store open: 8:00 AM (minute 0). Store close: 8:00 PM (minute 720).

DayTimer struct:
  u16 minute      — 0 to 720
  u8  hour        — 8 to 20
  u8  minute_of_hour — 0 to 59
  u8  day         — starts at 1
  u8  store_open  — flag
  u16 frame_accum — counts up to 4 then ticks minute

daytimer_init(): minute=0, day=1, store_open=1.
daytimer_update(): increment frame_accum each call. When frame_accum>=4:
  reset accum, increment minute, update hour and minute_of_hour, if minute>=720
  call daytimer_end_of_day().
daytimer_end_of_day(): set store_open=0, reset units_sold_today on all
products, increment day, reset minute to 0, set store_open=1 next frame.
daytimer_get_time_str(char* buf): writes "HH:MM AM/PM" into buf (11 chars).
```

**REVIEW P1-08**

```
Review daytimer.c and daytimer.h for GBA C.
Check: 4 frames per in-game minute, store open 0-719 minutes (720 = close),
hour derived as 8+(minute/60), minute_of_hour as minute%60, end_of_day resets
minute to 0 and increments day, units_sold_today reset on all products at
end of day, time string formats correctly as 12hr AM/PM with leading zero on
minutes, store_open flag set correctly. Fix bugs. Output corrected files only.
```

-----

### P1-09 · HUD SYSTEM

```
GBA C. libtonc. Write src/ui/hud.c and include/hud.h.

HUD lives on BG0, fixed, top 10px = top row of 8x8 tiles (y=0, 30 tiles wide).
BG0 charblock 0 mapblock 31. Font tiles must be loaded into charblock 0.

hud_init(): set BG0 for fixed display. Fill top tile row with dark bar tile.
hud_draw(): called every frame. Write these 3 zones as tile glyphs into BG0 map:
  Zone 1 cols 0-9:  "$XXX.XX" from g_cash (convert cents to dollar string)
  Zone 2 cols 10-16: "BOX:XX" from g_backroom_total
  Zone 3 cols 17-22: "DAY X" from g_daytimer.day
  Zone 4 cols 23-29: time string from daytimer_get_time_str()

Write a minimal 8x8 pixel font into VRAM for digits 0-9, letters A-Z, $:./
space. Font can be hardcoded as u32 arrays (4 bytes per row, 8 rows = 32 bytes
per glyph). Glyphs written as 1bpp into 4bpp tile slots using palette index 1
for lit pixels, 0 for background.
```

**REVIEW P1-09**

```
Review hud.c and hud.h for GBA C.
Check: BG0 not scrolled, font tiles written to correct charblock not
overwriting game tiles, cent-to-dollar conversion correct (5000 cents =
"$50.00" not "$5000"), backroom total pulls from g_backroom_total,
day number pulls from daytimer, time string from daytimer_get_time_str,
glyph writes use correct VRAM offset formula for BG0 screenblock,
no floats (integer division for dollars, modulo for cents), no malloc.
Fix bugs. Output corrected files only.
```

-----

### P1-10 · NPC SYSTEM — SINGLE SHOPPER

```
GBA C. libtonc OAM. Write src/npc/npc.c and include/npc.h.

NPC state enum: NPC_IDLE, NPC_NAVIGATE, NPC_AT_SHELF, NPC_QUEUE, NPC_PAYING,
NPC_EXITING.

NPC struct:
  u8 x, y           — tile pos
  u8 px, py         — pixel pos
  u8 dir
  u8 state
  u8 shopping_list[3] — product IDs (0xFF = empty slot)
  u8 list_size
  u8 list_index      — current item being sought
  u8 cart_count      — items in cart
  u16 cart_total     — cents owed
  u8 patience        — countdown timer in seconds (in-game), starts 300
  u8 wp_index        — current waypoint index in active route
  u8 anim_frame
  u8 anim_timer
  u8 active          — is this NPC slot in use

Waypoint routes (hardcoded arrays of tile coords):
  ROUTE_SHELF1[]: spawn(7,9) → aisle approach → (4,3) stop
  ROUTE_SHELF2[]: current pos → (9,3) stop
  ROUTE_REGISTER[]: current pos → (7,8) stop
  ROUTE_EXIT[]: (7,8) → (7,9) stop

npc_init(): clear all NPC slots.
npc_spawn(): find free slot, set active=1, place at (7,9), generate shopping
list of 1-3 random product IDs from g_products[0..2], set state=NPC_NAVIGATE,
set wp_index=0, set route to ROUTE_SHELF1. Use lcg rand seeded from day+minute.

npc_update(NPC* n): move along current waypoint route 1 tile per 24 frames.
On reaching waypoint: advance wp_index. On reaching final waypoint for state:
  NPC_NAVIGATE+at shelf: check inventory stock. If stock>0: call
    inventory_sell(product_id), increment cart_count, add price to cart_total,
    advance list_index, if more items set next route, else route to register,
    set state=NPC_QUEUE.
  If stock=0: skip item, advance list_index, if more items continue, else queue.
  NPC_QUEUE+at register: set state=NPC_PAYING. Set g_customer_waiting=1.
  NPC_PAYING: wait for player to call npc_checkout(n).
  NPC_EXITING+at exit tile: set active=0.

npc_checkout(NPC* n): add cart_total to g_cash. Set state=NPC_EXITING.
Set route to ROUTE_EXIT. Set g_customer_waiting=0.

npc_draw(NPC* n): OAM entries 2+3 for NPC body, entry 4 for cart sprite.
Cart offset +8px right. Cart tile: 0 items=empty frame, 1=one item, 2+=full.

Global: u8 g_customer_waiting=0. u16 g_npc_spawn_timer. Spawn 1 NPC every
2700 frames (45 seconds at 60fps) if no NPC active and store open.
```

**REVIEW P1-10**

```
Review npc.c and npc.h for GBA C libtonc.
Check: NPC moves 1 tile per 24 frames not 1 per frame, waypoint routes are
hardcoded tile coord arrays not computed paths, inventory_sell called when NPC
reaches shelf (not when they arrive — check stock first), cart_total
accumulates correctly in cents, g_customer_waiting set to 1 when NPC at
register, npc_checkout adds cart_total to g_cash not per-item, patience
decrements each in-game minute while in NPC_QUEUE state and NPC abandons if
patience reaches 0, NPC despawns (active=0) on reaching exit tile,
spawn timer resets after spawning, OAM entries don't conflict with player
entries 0+1, no floats, no malloc. Fix bugs. Output corrected files only.
```

-----

### P1-11 · INVENTORY SCREEN UI

```
GBA C. libtonc. Write src/ui/inventory_screen.c and include/inventory_screen.h.

Triggered by SELECT. Full-screen overlay on BG1 (swap BG1 to show menu tiles).
Pause game loop while open. B closes.

Screen layout (text tiles on BG1):
  Row 0: "=== INVENTORY ===" centered
  Row 2: "PRODUCT     SHELF BACK  COST  ORDER"
  Rows 3-5: one row per product showing name, stock_on_shelf, stock_in_back,
    cost_price as "$X.XX", and order quantity selector "> 0 <"
  Row 7: "CASH: $XXX.XX"
  Row 8: "A=ORDER  B=CLOSE  UP/DN=SELECT  LFT/RGT=QTY"

Cursor selects product row. Left/right adjusts order qty (0-12). A confirms
order for selected product+qty: calls inventory_order(id, qty). Show "OK" or
"NO CASH" feedback for 60 frames. If no NPC active and order placed, items
in backroom immediately (demo = instant delivery).

No BG mode switch needed — just write text glyphs to BG1 screenblock while
screen is open, restore map tiles on close.
```

**REVIEW P1-11**

```
Review inventory_screen.c and inventory_screen.h for GBA C.
Check: SELECT opens, B closes, game loop paused while open (no player/NPC
updates), cursor wraps within 3 product rows, order qty clamped to 0-12,
A calls inventory_order and checks return value for cash feedback,
cost shown per unit not total, BG1 tiles restored from demo_map_bg1 on close,
all values pulled from g_products[] not hardcoded, g_cash displayed correctly
as dollar string. Fix bugs. Output corrected files only.
```

-----

### P1-12 · CHECKOUT INTERACTION

```
GBA C. Write src/ui/checkout.c and include/checkout.h.

Triggered when: player presses A at register tile AND g_customer_waiting==1.

checkout_start(): read active NPC's shopping_list and cart_total.
Show full-screen text overlay on BG1:
  "CHECKOUT"
  List each item: "Bread x1  $1.49"
  "TOTAL: $X.XX"
  "A=COLLECT"

checkout_confirm(): player presses A. Call npc_checkout(active_npc).
Show "+$X.XX" float sprite on screen for 60 frames (OAM slot 5, animate
upward 1px per 2 frames, then set inactive). Close overlay. Restore BG1.

If player at register and g_customer_waiting==0, show "NO CUSTOMER" for
30 frames then dismiss.
```

**REVIEW P1-12**

```
Review checkout.c and checkout.h for GBA C.
Check: checkout only triggers if g_customer_waiting==1 AND player at register
tile (RG, bit3 set), cart total pulled from NPC struct not recalculated,
npc_checkout called exactly once on confirm (not on overlay open), float sprite
uses temporary OAM slot cleared after animation, BG1 restored to map tiles
after close, "NO CUSTOMER" guard present, no floats, no malloc.
Fix bugs. Output corrected files only.
```

-----

### P1-13 · SRAM SAVE SYSTEM

```
GBA C. Write src/save/save.c and include/save.h.

GBA SRAM at 0x0E000000. 32KB. 3 save slots, each 10KB max.
Access SRAM byte-by-byte only (8-bit reads/writes, never 16 or 32 bit).

SaveSlot struct (must fit in ~10KB):
  u16 day_number
  u32 cash_balance    (cents)
  u8  store_level     (always 1 in demo)
  u8  reputation      (always 0 in demo)
  u32 total_revenue   (cents)
  char store_name[17]
  struct { u8 on_shelf; u8 in_back; u16 sold_total; } product_save[32]
  u8  checksum        (XOR of all bytes in slot)

save_write(u8 slot): serialize GameState + g_products + g_cash into SaveSlot.
Write byte-by-byte to SRAM at offset slot*10240. Compute and write checksum.

save_read(u8 slot): read SaveSlot from SRAM. Verify checksum. If valid,
restore g_cash, g_products stock values, day number. Return 1 on success,
0 on checksum fail.

save_slot_exists(u8 slot): read checksum byte, verify against data. Return 1
if valid slot present.

save is called automatically at end of day (daytimer_end_of_day hook).
```

**REVIEW P1-13**

```
Review save.c and save.h for GBA C SRAM.
Check: SRAM accessed byte by byte only (no u16/u32 SRAM writes — GBA SRAM
is 8-bit bus), slot offset math correct (slot 0=0, slot 1=10240, slot 2=20480),
SaveSlot fits within 10240 bytes, checksum computed over all bytes before
checksum field, checksum verified on read before restoring state, cash and
stock values correctly serialized/deserialized, checksum fail returns 0 and
does not corrupt live game state. Fix bugs. Output corrected files only.
```

-----

### P1-14 · MAIN GAME LOOP

```
GBA C. Write src/main.c final version.

main():
  irq_init(NULL). irq_enable(II_VBLANK).
  tile_engine_init().
  demo_map_load().
  inventory_init().
  daytimer_init().
  hud_init().
  npc_init().
  input_poll() once to clear state.

  Check save_slot_exists(0). If yes, save_read(0).

  Main loop (while 1):
    VBlankIntrWait().
    input_poll().

    if store_open:
      if SELECT just pressed: open inventory_screen, wait for close.
      if A just pressed at register: checkout flow.
      else: player_update().
      daytimer_update().
      npc_spawn check (timer-based).
      for each active NPC: npc_update().

    hud_draw().
    for each active NPC: npc_draw().
    player_draw().

All per-frame logic between VBlankIntrWait calls. All drawing writes to OAM
and BG map only inside vblank window. HUD drawn every frame regardless of
store open state.
```

**REVIEW P1-14**

```
Review main.c game loop for GBA C.
Check: VBlankIntrWait called at top of loop (not bottom), input_poll before
any state reads, player_update not called while inventory_screen or checkout
overlay is open, daytimer_update called every frame while store open,
npc spawner checks store_open and active NPC count before spawning, all OAM
and VRAM writes happen inside vblank window not during active display, save
loaded on boot if slot exists, IRQ init correct for libtonc. Fix bugs.
Output corrected files only.
```

-----

### P1-FINAL · INTEGRATION REVIEW

```
You are reviewing a complete GBA C project for Phase 1 demo. It uses
devkitARM, libtonc, Mode 0.

Review the full codebase for:
1. Any VRAM/OAM writes outside vblank window
2. Any float usage
3. Any malloc/free usage
4. Any 16 or 32-bit SRAM writes (must be 8-bit only)
5. OAM slot conflicts between player (0-1), NPC body (2-3), cart (4),
   float sprite (5)
6. g_collision_flags not populated before player or NPC collision checks
7. g_cash ever going below 0 (must be guarded in inventory_order)
8. NPC and player sharing tile position (no occupancy check exists — note
   this as known limitation for Phase 2)
9. End of day not saving before resetting daily stats

For each issue found: file name, line range, description, fix.
After listing issues, output a PASS or FAIL verdict for demo readiness.
```

-----

## PHASE 2 — CORE TYCOON LOOP

-----

### P2-00 · COORDINATOR

```
Phase 1 demo is complete and compiles. Phase 2 adds to the existing codebase.
Do not rewrite Phase 1 files unless a P2 task explicitly says to modify one.

Phase 2 adds: pricing controls, market price fluctuation, delayed delivery
orders, 4 simultaneous NPCs, shopper patience + satisfaction scoring, end of
day summary screen, 12 total products, store reputation stat.

Output a task list for Phase 2 in build order. For each task: which files are
new, which Phase 1 files are modified, and what the dependency is. No code.
```

-----

### P2-01 · PRICING CONTROLS

```
Modify src/ui/inventory_screen.c. Add per-product shelf price editor.

In inventory screen, add second mode toggled by START: PRICE MODE.
In price mode: cursor selects product. Left/right adjusts shelf_price by 5
cents per press. Floor = cost_price + (cost_price/10). Ceiling = market_price*2.
Clamp on adjust. Show live margin: "(+$X.XX)" next to price.
A confirms and writes new shelf_price to g_products[id].shelf_price.
HUD tile for that shelf updates next npc_draw call (shelf label tile).

Add to hud: small price label tile per shelf showing shelf_price. Update
when price changes. Tile is 3 chars wide: "$X.X" truncated.
```

**REVIEW P2-01**

```
Review pricing control changes to inventory_screen.c for GBA C.
Check: price floor = cost + cost/10 using integer division, price ceiling =
market_price*2, both clamps applied on left/right press not just on confirm,
margin shown as shelf_price - cost_price in cents then displayed as dollar
string, A writes to g_products not a local copy, no floats anywhere in margin
calc or clamp. Fix bugs. Output modified file only.
```

-----

### P2-02 · MARKET PRICE FLUCTUATION

```
GBA C. Write src/core/market.c and include/market.h.

market_init(): set g_products[i].market_price = g_products[i].shelf_price
for all products.

market_daily_update(): called in daytimer_end_of_day. For each product:
  Generate random delta: -10 to +10 cents using lcg_rand().
  Apply: market_price = clamp(market_price + delta, cost_price, cost_price*3).
  5% chance per product of a supply event: delta = +20 to +40 cents.
  Store market_price changes in g_market_history[MAX_PRODUCTS][7] (7-day
  rolling window, shift on each update). All in EWRAM.

market_get_event_str(u8 product_id, char* buf): if today's delta > 15,
write event string like "BREAD UP!" into buf (10 chars max). Else empty string.

Expose g_market_history for ledger screen use in Phase 4.
```

**REVIEW P2-02**

```
Review market.c and market.h for GBA C.
Check: market_price never falls below cost_price, never exceeds cost_price*3,
delta uses integer LCG rand not float random, 7-day history array in EWRAM
section, shift-on-update means history[0] is always today, supply event check
is 1-in-20 chance using (rand%20==0) not float probability, event string fits
10 chars, market_daily_update called after daily stats reset not before.
Fix bugs. Output corrected files only.
```

-----

### P2-03 · DELAYED DELIVERY ORDERS

```
Modify src/core/inventory.c. Replace instant delivery with order queue.

Order struct (already in constants: MAX_ORDERS=8):
  u8 product_id
  u8 qty
  u8 deliver_day   — day number when items arrive in backroom
  u8 fulfilled     — flag

Global: Order g_pending_orders[MAX_ORDERS] in EWRAM.

inventory_order(): instead of instant backroom add, find free order slot,
set deliver_day = current_day + 1. If order placed after in-game minute 540
(= 9PM, but store closes at 720 so cutoff is minute 600 = 5PM): deliver_day
+= 1 more. Deduct cash immediately on order. Return 0 if no free order slot.

inventory_check_deliveries(): called in daytimer_init of new day (after day
increment). For each order where deliver_day == current_day and fulfilled==0:
add qty to product.stock_in_back, add qty to g_backroom_total, set fulfilled=1.

Inventory screen: show pending orders below product list. Each row: product
name, qty, "ARR DAY X".
```

**REVIEW P2-03**

```
Review delayed delivery changes to inventory.c for GBA C.
Check: cash deducted at order time not delivery time, deliver_day = day+1
for orders before cutoff minute 600, deliver_day = day+2 for after cutoff,
inventory_check_deliveries called at start of new day before player can act,
fulfilled flag prevents double-delivery, MAX_ORDERS=8 never exceeded (return 0
on full), g_pending_orders in EWRAM, inventory screen shows pending rows
without overflowing 10-tile height limit. Fix bugs. Output modified files only.
```

-----

### P2-04 · MULTI-NPC SYSTEM (4 MAX)

```
Modify src/npc/npc.c. Expand from 1 NPC to 4 simultaneous.

g_npcs[MAX_NPCS] already declared. Phase 1 only used slot 0.

npc_update_all(): loop all 8 slots, call npc_update on each active one.
npc_draw_all(): loop active slots. OAM layout: player=0-1, npc[0]=2-3+4,
npc[1]=5-6+7, npc[2]=8-9+10, npc[3]=11-12+13. Cart is always third entry
in each NPC's OAM block.

Spawn logic: max 4 active at once. Spawn timer fires every 2700 frames.
On spawn, pick random personality from: BARGAIN(0), LOYAL(1), IMPULSE(2),
RUSHED(3), FAMILY(4). Store as npc.personality u8.

Register queue: g_register_queue[4] is array of NPC pointers in order of
arrival. First in queue gets served. checkout triggers on queue[0].
After checkout, shift queue left.

Patience: RUSHED starts at 180 (3 min), others at 300 (5 min). Decrement
each in-game minute while in NPC_QUEUE state. On 0: mark NPC as left without
paying, set active=0, decrement reputation by 1.
```

**REVIEW P2-04**

```
Review multi-NPC changes to npc.c for GBA C.
Check: OAM slots correctly assigned per NPC index (no overlap with player 0-1),
max 4 active NPCs enforced at spawn (count active slots before spawning),
register queue is FIFO (first arrived served first), checkout always acts on
queue[0] not arbitrary NPC, queue shift-left after checkout, patience
decrements only while in NPC_QUEUE not during navigation, reputation deducted
on patience timeout not on empty shelf, personality stored but only patience
and impulse behavior wired in Phase 2 (others Phase 3), no floats, no malloc.
Fix bugs. Output modified file only.
```

-----

### P2-05 · SHOPPER SATISFACTION + REPUTATION

```
GBA C. Write src/core/reputation.c and include/reputation.h.

Global: u8 g_reputation = 50 (0-100 scale). In IWRAM.
Global: u8 g_daily_satisfaction[8] — one score per NPC slot, reset each day.

Satisfaction score per shopper (computed at checkout or abandon):
  Base: 50
  +10 per item successfully bought
  -10 per item skipped (empty shelf)
  -15 if waited > 120 in-game minutes in queue
  -5 per 20 cents above market price per item purchased
  Clamp 0-100.

reputation_update_from_npc(NPC* n): compute satisfaction, store in
g_daily_satisfaction[n->slot_index], call reputation_adjust.

reputation_adjust(s8 delta): apply delta to g_reputation, clamp 0-100.

reputation_daily_update(): called end of day. Average g_daily_satisfaction
for NPCs that visited. If avg > 70: g_reputation += 2. If avg < 40: -= 2.
Clamp 0-100. Reset g_daily_satisfaction[].

Reputation effects (checked in npc spawn):
  <30: max 1 NPC per day. 30-60: max 2. 60-80: max 3. >80: max 4.
```

**REVIEW P2-05**

```
Review reputation.c and reputation.h for GBA C.
Check: g_reputation clamped 0-100 after every adjust, satisfaction computed
using integer math (no floats), price penalty uses (shelf_price - market_price)
in cents divided by 20 using integer division, reputation_daily_update averages
only NPCs who actually visited (not all 8 slots), spawn cap correctly uses
reputation thresholds, reputation_update_from_npc called at checkout AND on
patience timeout abandon, g_daily_satisfaction reset at end of day.
Fix bugs. Output corrected files only.
```

-----

### P2-06 · 12 PRODUCT CATALOG + CATEGORIES

```
Modify src/core/inventory.c. Expand from 3 to 12 products.

Add to inventory_init():
  3: "Eggs",    cost=180, shelf_price=349, capacity=6,  cat=DAIRY
  4: "Cheese",  cost=220, shelf_price=429, capacity=4,  cat=DAIRY
  5: "Butter",  cost=150, shelf_price=299, capacity=6,  cat=DAIRY
  6: "Pasta",   cost=60,  shelf_price=129, capacity=8,  cat=DRY
  7: "Cereal",  cost=130, shelf_price=269, capacity=6,  cat=DRY
  8: "Soup",    cost=70,  shelf_price=149, capacity=8,  cat=DRY
  9: "Chicken", cost=280, shelf_price=549, capacity=4,  cat=MEAT
  10:"OJ",      cost=150, shelf_price=299, capacity=6,  cat=BEVERAGE
  11:"Water",   cost=40,  shelf_price=99,  capacity=10, cat=BEVERAGE

Category enum: PRODUCE=0, DAIRY=1, BAKERY=2, MEAT=3, FROZEN=4, DRY=5,
BEVERAGE=6, HEALTH=7.

Inventory screen: paginate — show 5 products per page. LB/RB (L/R shoulder
buttons) flip pages. Show page X/Y indicator. NPC shopping lists now
randomly pick from all active products (those with shelf>0 or back>0).
If product has 0 stock anywhere and no pending order, NPC won't add it to list.
```

**REVIEW P2-06**

```
Review expanded product catalog changes for GBA C.
Check: all 12 products initialized in inventory_init with correct cents values,
category enum matches header, inventory screen pagination shows exactly 5 rows
per page with L/R shoulder buttons (KEY_L and KEY_R masks), NPC list generation
filters to only products with stock > 0 somewhere or pending order, no product
ID ever exceeds MAX_PRODUCTS=32, EWRAM section still applied to g_products[].
Fix bugs. Output modified files only.
```

-----

### P2-07 · END OF DAY SUMMARY SCREEN

```
GBA C. Write src/ui/eod_screen.c and include/eod_screen.h.

Called from daytimer_end_of_day() before resetting daily stats.
Blocks main loop until player presses A.

Screen layout (BG1 text overlay):
  Row 0:  "=== END OF DAY X ==="
  Row 2:  "REVENUE:   $XXX.XX"
  Row 3:  "EXPENSES:  $XXX.XX"
  Row 4:  "PROFIT:    $XXX.XX" (revenue - expenses, can be negative)
  Row 6:  "CUSTOMERS: XX"
  Row 7:  "ITEMS SOLD: XX"
  Row 9:  "REPUTATION: XX/100"
  Row 10: market event strings for any products with big price swings
  Row 13: "A=NEXT DAY"

Track daily: g_daily_revenue (u32 cents), g_daily_expenses (u32 cents),
g_daily_customers (u8), g_daily_items_sold (u8). Reset after screen dismissed.
Revenue incremented in inventory_sell. Expenses incremented in inventory_order.
Customers incremented in npc_checkout. Items sold = sum of units_sold_today.
```

**REVIEW P2-07**

```
Review eod_screen.c and eod_screen.h for GBA C.
Check: screen called before daily stats reset not after, A press required
to advance (not automatic timer), profit shown as revenue-expenses and can
display negative (use s32 for profit calc), all dollar strings use integer
cent division, reputation shows g_reputation not raw satisfaction, market
event strings pulled from market_get_event_str for each product, BG1
restored after dismiss, daily tracking variables reset after dismiss not
inside eod function. Fix bugs. Output corrected files only.
```

-----

### P2-FINAL · INTEGRATION REVIEW

```
Review Phase 2 additions to GBA C codebase.

Check:
1. Market price never below any product's cost_price after daily update
2. Delayed orders: cash deducted at order but items not available until
   deliver_day — confirm no way to stock items before delivery
3. OAM slots 0-13 correctly assigned across player + 4 NPCs + 4 carts
4. g_register_queue shift-left on checkout leaves no stale pointers
5. Reputation spawn cap enforced before spawning not after
6. EOD screen called before stat reset and save called after EOD screen
7. Inventory screen pagination doesn't access g_products beyond index 11
8. All new EWRAM globals have __attribute__((section(".ewram")))
9. No new floats introduced in market, reputation, or satisfaction math

List all issues with file + line range + fix. Output PASS or FAIL.
```

-----

## PHASE 3 — GROWTH & DELEGATION

-----

### P3-00 · COORDINATOR

```
Phase 2 complete. Phase 3 adds: 2-room store with camera scroll, employee
system (hire/assign/pay wages), department organization, shelf purchasing,
employee stocking AI, up to 20 products.

Output task list in build order. For each task: new files, modified files,
dependency. Camera scroll requires tile engine changes. Employee system requires
new AI loop alongside NPC loop. Shelf purchasing requires map mutation at
runtime. No code — task list only.
```

-----

### P3-01 · CAMERA + MAP SCROLL

```
Modify src/gfx/tile_engine.c and src/map/. Add camera system.

Camera struct: s16 cam_x, cam_y in pixels. World map now 30 wide x 10 tall
(2 rooms side by side). VRAM screenblock holds 32x32 tiles — world map fits.

tile_engine_set_camera(s16 x, s16 y): clamp x to 0..((MAP_W_WORLD-15)*16),
clamp y to 0..0 (vertical locked). Write clamped x to BG1/BG2/BG3 HOFS.
BG0 HOFS never touched (HUD fixed).

player_update(): after move, compute target cam_x = player.px - 112 (center
player). Lerp cam_x toward target by 4px per frame using integer approach:
  if cam_x < target: cam_x = min(cam_x+4, target)
  else: cam_x = max(cam_x-4, target)
Call tile_engine_set_camera each frame.

World map room 2 (cols 15-29) layout: define in demo_map.c as second half
of map arrays. Same tile ID scheme. Expand map arrays to [300] (30x10).
```

**REVIEW P3-01**

```
Review camera scroll changes for GBA C.
Check: cam_x clamped to prevent showing outside world bounds, BG0 HOFS never
written, BG1/BG2/BG3 all receive same HOFS value (no parallax in Phase 3),
lerp is integer-only (no floats), player pixel position relative to camera
correctly computed for OAM (OAM x = player.px - cam_x), NPC OAM also offset
by cam_x, world map array is exactly 300 bytes for 30x10, screenblock writes
cover full 30-wide world. Fix bugs. Output modified files only.
```

-----

### P3-02 · RUNTIME SHELF PLACEMENT

```
GBA C. Write src/map/shelf_manager.c and include/shelf_manager.h.

Player can buy a new shelf from the inventory screen (new tab).
Shelf cost: 500 cents ($5.00). Max 32 shelves (MAX_SHELVES).

shelf_buy(u8 world_x, u8 world_y): deduct 500 from g_cash. Set tile at
(world_x, world_y) in world map to SH (tile ID 32). Set tile at
(world_x, world_y+1) to S_EMPTY (tile ID 32). Write to VRAM screenblock
immediately. Update g_collision_flags for those positions. Add to g_shelves[]
array with product_id=0xFF (unassigned). Return 0 if position already solid or
cash insufficient.

shelf_assign_product(u8 shelf_index, u8 product_id): link shelf slot to product.

Placement UI: in inventory screen new "SHELVES" tab. Show world map as 30x10
minimap (1 tile per character). Cursor moves on minimap. A places shelf at
cursor position if tile is FL and not adjacent to another shelf. Preview shows
proposed shelf tile highlighted.
```

**REVIEW P3-02**

```
Review shelf_manager.c and shelf_manager.h for GBA C.
Check: cash deducted before tile write (not after), VRAM write uses correct
screenblock offset for world_x > 14 (room 2), collision flags updated for both
header tile and shelf tile (header=solid+interact, shelf=solid+interact+shelf),
g_shelves[] not exceeded MAX_SHELVES=32, minimap renders at 1 char per tile
using correct world map data not just room 1, adjacent-shelf check prevents
overlapping placements, shelf with product_id=0xFF not checked by NPC for
shopping. Fix bugs. Output corrected files only.
```

-----

### P3-03 · EMPLOYEE DATA + HIRE SYSTEM

```
GBA C. Write src/core/employee.c and include/employee.h.

Employee struct:
  u8  id
  char name[10]
  u8  role         — 0=STOCKER, 1=CASHIER (Phase 3 only 2 roles)
  u16 daily_wage   — cents
  u8  assigned_shelf — shelf index they restock
  u8  active

Global: Employee g_employees[8] in EWRAM.
Global: u16 g_daily_wages — sum of all active employee wages, deducted EOD.

employee_hire(u8 role): find free slot. Assign default name ("CLERK X").
STOCKER wage=800 cents/day ($8.00). CASHIER wage=1000 cents/day ($10.00).
Set active=1. Add wage to g_daily_wages.

employee_fire(u8 id): set active=0. Recompute g_daily_wages.

Hire UI: in inventory screen "STAFF" tab. Show slots. A to hire role at
cursor. B to fire selected active employee. Show daily cost impact:
"DAILY COST: +$X.XX".

EOD: deduct g_daily_wages from g_cash. If g_cash would go below 0: allow it
(debt, shown in red on HUD — negative cash display). Add wages to
g_daily_expenses.
```

**REVIEW P3-03**

```
Review employee.c and employee.h for GBA C.
Check: MAX 8 employees enforced (return fail if all slots active), daily wages
deducted at EOD not per frame, g_daily_wages recomputed on hire and fire (not
incrementally tracked to avoid drift), debt allowed (g_cash is u32 but treat
as signed for display — or use s32 for g_cash in Phase 3 since wages can
overdraft), STOCKER assigned a shelf index before they act (AI won't run if
assigned_shelf=0xFF), CASHIER role stored but AI not wired until P3-04,
EOD wage deduction added to g_daily_expenses for EOD screen. Fix bugs.
Output corrected files only.
```

-----

### P3-04 · EMPLOYEE AI — STOCKER

```
GBA C. Write src/npc/employee_ai.c and include/employee_ai.h.

Stocker AI runs alongside NPC loop. Uses OAM slots 14-15 per employee (up to
4 stockers visible). Shares NPC movement code (same waypoint system).

EmployeeAI struct:
  u8 emp_id
  u8 x, y, px, py, dir
  u8 state    — IDLE, WALK_TO_BACK, WALK_TO_SHELF, STOCKING, WALK_IDLE
  u8 target_shelf
  u8 anim_frame, anim_timer

employee_ai_update(u8 emp_id):
  Employee* e = &g_employees[emp_id].
  If role != STOCKER or !active: return.
  Shelf* s = &g_shelves[e->assigned_shelf].
  If s->stock_on_shelf < s->shelf_capacity AND s->product->stock_in_back > 0:
    if state==IDLE: set state=WALK_TO_SHELF, compute waypoints to shelf.
    if state==WALK_TO_SHELF and at shelf: call inventory_stock_shelf(product),
      set state=IDLE.
  If s->stock_in_back == 0 and s->stock_on_shelf < s->shelf_capacity:
    stocker does nothing (no backroom stock to pull from).

employee_ai_update_all(): loop g_employees, call update per active stocker.
Called in main loop alongside npc_update_all.
```

**REVIEW P3-04**

```
Review employee_ai.c and employee_ai.h for GBA C.
Check: stocker only moves if assigned shelf has stock_in_back > 0 AND
shelf is not full, stocker calls inventory_stock_shelf not a custom function
(reuses Phase 1 inventory logic), OAM slots 14-15 per employee don't overlap
NPC slots (NPCs use 2-13 for 4 NPCs), stocker state machine has no infinite
loop (IDLE when nothing to do), stocker respects collision flags on movement
(same g_collision_flags check as player), employee_ai not called if
g_employees[i].role != STOCKER. Fix bugs. Output corrected files only.
```

-----

### P3-05 · 20 PRODUCT CATALOG EXPANSION

```
Modify src/core/inventory.c. Expand to 20 products.

Add products 12-19:
  12:"Yogurt",   cost=120, shelf_price=239, capacity=6,  cat=DAIRY
  13:"Ice Cream",cost=200, shelf_price=399, capacity=4,  cat=FROZEN
  14:"Pizza",    cost=180, shelf_price=349, capacity=4,  cat=FROZEN
  15:"Shampoo",  cost=150, shelf_price=299, capacity=6,  cat=HEALTH
  16:"Soap",     cost=80,  shelf_price=169, capacity=8,  cat=HEALTH
  17:"Beef",     cost=350, shelf_price=699, capacity=4,  cat=MEAT
  18:"Bananas",  cost=40,  shelf_price=89,  capacity=10, cat=PRODUCE
  19:"Tomatoes", cost=60,  shelf_price=129, capacity=8,  cat=PRODUCE

NPC shopping list generation: filter by department if NPC personality is
FAMILY (picks from at least 3 different categories). Others still random.

Inventory screen: add category filter. L/R shoulder cycles category filter
(ALL, DAIRY, DRY, PRODUCE, etc). Page within filtered view.
```

**REVIEW P3-05**

```
Review 20-product expansion for GBA C.
Check: products 12-19 added with correct cent values, category filter in
inventory screen uses L shoulder to cycle (not overwrite P2 pagination behavior
— reconcile L/R usage between pagination and filtering), FAMILY NPC list
generation picks from >= 3 categories using category field not hardcoded IDs,
all 20 products have market_price initialized equal to shelf_price in
inventory_init, no product ID exceeds MAX_PRODUCTS=32. Fix bugs. Output
modified files only.
```

-----

### P3-FINAL · INTEGRATION REVIEW

```
Review Phase 3 additions to GBA C codebase.

Check:
1. Camera OAM offset applied to both NPCs and employees (not just player)
2. Stocker AI and NPC AI not calling same inventory functions concurrently
   (GBA is single-threaded so verify call order in main loop prevents race)
3. Shelf purchased at runtime: VRAM write, collision flag update, g_shelves[]
   entry — all 3 happen or none (atomic-ish, no partial state)
4. Employee wages deducted once per day not once per frame
5. g_cash changed to s32 or negative display handled correctly after debt
6. World map 30x10 = 300 bytes — confirm map arrays are exactly 300
7. Employee OAM slots 14+ don't exceed OAM limit of 128 entries total
8. Save system updated to persist: g_employees[], g_shelves[] runtime state,
   g_reputation, new product slots 12-19

List all issues with file + line + fix. Output PASS or FAIL.
```

-----

## PHASE 4 — FULL SIMULATION

-----

### P4-00 · COORDINATOR

```
Phase 3 complete. Phase 4 adds: delivery truck logistics with visual truck
arrival, rent and overhead daily expenses, equipment purchases (coolers for
dairy/frozen, display cases), seasonal events that affect market prices for
1-week windows, competitor NPC store that steals customers if reputation low,
40+ product catalog.

Output task list in build order. Flag which tasks require new map tiles,
new sprite frames, or new EWRAM structures. No code.
```

-----

### P4-01 · RENT + OVERHEAD SYSTEM

```
Modify src/core/daytimer.c and src/ui/eod_screen.c.

Add to EOD expenses:
  Rent: fixed 1000 cents/day ($10.00) starting day 8 (first week free).
  Utility: 200 cents/day base + 50 cents per active employee + 30 cents per
    refrigeration unit owned.
  Both deducted in daytimer_end_of_day after wages.
  Add to g_daily_expenses before EOD screen.

Global: u8 g_rent_active (set day>=8). u8 g_refrigeration_count.
Add rent and utility lines to EOD summary screen.
EOD screen: if total expenses > revenue, flash PROFIT line in alternate palette
color (palette swap trick: write different palette index to affected tiles).
```

-----

### P4-02 · EQUIPMENT PURCHASE SYSTEM

```
GBA C. Write src/core/equipment.c and include/equipment.h.

Equipment types: COOLER (enables dairy/frozen stocking, cost 3000 cents),
DISPLAY_CASE (increases shelf capacity+2 for assigned shelf, cost 2000 cents).

Equipment struct: u8 type, u8 assigned_shelf, u8 active.
Global: Equipment g_equipment[16] in EWRAM.

equipment_buy(u8 type, u8 shelf_index): deduct cost, set active, link to shelf.
COOLER: set g_refrigeration_count++. Required to stock cat=DAIRY or cat=FROZEN.
DISPLAY_CASE: shelf_capacity += 2 for linked shelf immediately.

Guard in inventory_stock_shelf: if product.category==DAIRY or FROZEN and no
active COOLER: return 0, show "NEED COOLER" message.

Add EQUIPMENT tab to inventory screen.
```

-----

### P4-03 · SEASONAL EVENTS

```
Modify src/core/market.c. Add seasonal event system.

Season enum: SPRING=0(days 1-30), SUMMER=31-60, FALL=61-90, WINTER=91-120.
After day 120 cycle repeats.

Season modifiers applied in market_daily_update:
  SUMMER: BEVERAGE market_price +15%, FROZEN +10%
  WINTER: FROZEN +20%, DRY +5%
  FALL:   PRODUCE -10% (harvest surplus), BAKERY +5%
  SPRING: PRODUCE +10%
All modifiers as integer percent applied with (price * pct) / 100.

Random weekly event (once per 7 days, 1 random product):
  "SHORTAGE": market_price += 40-80 cents for 7 days.
  "SURPLUS":  market_price -= 20-40 cents for 7 days.
Track with: u8 g_event_product, u8 g_event_days_left, s8 g_event_delta.
Tick g_event_days_left each day, remove modifier when 0.

Show active event on HUD as scrolling ticker on BG0 row 1 (below main HUD).
```

-----

### P4-04 · 40 PRODUCT CATALOG

```
Modify src/core/inventory.c. Expand to 40 products.

Add products 20-39 across all 8 categories. Include:
  20:"Salmon"   cat=MEAT,    cost=400, price=799, cap=4
  21:"Tuna Can" cat=DRY,     cost=90,  price=189, cap=8
  22:"Juice"    cost=120,    price=249, cap=6,  cat=BEVERAGE
  23:"Soda"     cost=60,     price=129, cap=10, cat=BEVERAGE
  24:"Cake"     cat=BAKERY,  cost=300, price=599, cap=4
  25:"Muffins"  cat=BAKERY,  cost=150, price=299, cap=6
  26:"Spinach"  cat=PRODUCE, cost=70,  price=149, cap=8
  27:"Peppers"  cat=PRODUCE, cost=80,  price=169, cap=8
  28:"Cream"    cat=DAIRY,   cost=140, price=279, cap=6
  29:"Kefir"    cat=DAIRY,   cost=160, price=319, cap=4
  30:"Peas"     cat=FROZEN,  cost=90,  price=189, cap=8
  31:"Waffles"  cat=FROZEN,  cost=130, price=269, cap=6
  32:"Rice"     cat=DRY,     cost=80,  price=169, cap=8
  33:"Oats"     cat=DRY,     cost=100, price=209, cap=8
  34:"Vitamins" cat=HEALTH,  cost=200, price=399, cap=4
  35:"Toothpaste"cat=HEALTH, cost=110, price=229, cap=6
  36:"Lamb"     cat=MEAT,    cost=450, price=899, cap=4
  37:"Turkey"   cat=MEAT,    cost=380, price=749, cap=4
  38:"Energy"   cat=BEVERAGE,cost=100, price=219, cap=8
  39:"Tea"      cat=BEVERAGE,cost=70,  price=149, cap=10
All names 12 chars max. All values in cents.
```

-----

### P4-FINAL · INTEGRATION REVIEW

```
Review Phase 4 additions to GBA C codebase.

Check:
1. Seasonal modifier uses integer percent math (price*pct/100) not floats
2. Weekly event delta correctly removed after g_event_days_left reaches 0
3. Rent deducted starting day 8 not day 1 (g_rent_active flag)
4. Equipment COOLER guard fires in inventory_stock_shelf not in buy function
5. 40 products fit in g_products[MAX_PRODUCTS=32] — MAX_PRODUCTS must be
   raised to 48 in constants.h and all fixed arrays updated
6. EWRAM budget: estimate bytes used by g_products[48] + g_equipment[16] +
   g_pending_orders[8] + g_market_history[48][7] — confirm under 256KB
7. HUD ticker (BG0 row 1) doesn't overwrite main HUD row 0
8. Save system updated for new fields: g_equipment[], season, event state,
   rent_active flag, 48 product slots

List all issues. Output PASS or FAIL.
```

-----

## PHASE 5 — ENDGAME

-----

### P5-00 · COORDINATOR

```
Phase 4 complete. Phase 5 adds: multiple checkout registers (up to 3),
specialty departments with unique mechanics (deli counter = made-to-order,
bakery = timed baking, pharmacy = restricted items), store remodel system
(player rearranges tiles in edit mode), 50+ products, prestige win condition
"Grand Opening" triggered when reputation=100 + day>=60 + cash>=50000.

Output task list in build order. Flag any new sprite requirements, new BG layer
usage, or new save fields. No code.
```

-----

### P5-01 · MULTIPLE REGISTERS

```
Modify src/ui/checkout.c and src/npc/npc.c.

Add up to 3 register tiles to world map. Each register is an independent
NPC_QUEUE. Global: NPC* g_register_queues[3][4] — 3 queues of 4.

NPC routing: when list complete, NPC picks shortest queue (fewest active NPCs
in g_register_queues[i]). Navigate to that register's tile.

Player can stand at any register tile. Checkout acts on queue[0] of whichever
register the player is standing at.

CASHIER employee (P3 role, previously unused): if CASHIER assigned to a
register index, they auto-checkout queue[0] every 240 frames (4 in-game min)
without player input. Player still handles register 0 manually.

Register 2 and 3 unlock at store_level 3 (Phase 5 entry). Tile positions:
Register 0: (7,8) — existing. Register 1: (9,8). Register 2: (11,8).
Add tiles to world map array.
```

-----

### P5-02 · STORE REMODEL EDIT MODE

```
GBA C. Write src/map/remodel.c and include/remodel.h.

Edit mode: hold SELECT+START for 2 seconds to enter. Store closes (NPC spawning
paused, existing NPCs finish and exit). Overlay text: "REMODEL MODE".

In edit mode: D-pad moves cursor (32x8px highlight sprite on OAM 60). A picks
up tile under cursor (if it's a shelf group — SH+S pair). A again places at
new cursor position. B cancels pick. START+SELECT exits (resumes normal mode).

Rules: can only pick up shelf groups (SH+Sx tile pairs). Cannot place on solid
tiles. Cannot place blocking the path between door and any register (pathability
check: run simple flood fill from DR tile, if register tiles unreachable, reject
placement and show "BLOCKED").

On place: update world map arrays, write VRAM, update g_collision_flags for
moved positions, update g_shelves[] world_x/world_y for moved shelf.

Flood fill: BFS from door tile through non-solid tiles. If all register tiles
reachable: allow. BFS on 30x10=300 tiles is fast enough on GBA ARM7.
```

-----

### P5-03 · WIN CONDITION + PRESTIGE SCREEN

```
GBA C. Write src/ui/prestige.c and include/prestige.h.

Check win condition each EOD in daytimer_end_of_day:
  g_reputation >= 100 AND g_daytimer.day >= 60 AND g_cash >= 50000.

If met: trigger prestige_screen().

prestige_screen(): full BG takeover. Show:
  "GRAND OPENING!"
  "You built a supermarket!"
  "Days: XX  Revenue: $XXXXX.XX  Reputation: 100"
  "A=CONTINUE  START=TITLE"

CONTINUE: set g_cash += 20000 bonus, g_reputation = 80 (reset slightly),
unlock prestige flag in save, return to game. Player can keep playing.
TITLE: return to title screen, mark save slot as PRESTIGE cleared.

Prestige save flag: u8 prestige_cleared in SaveSlot. Display crown icon
next to prestige-cleared save slots on title screen.
```

-----

### P5-FINAL · FULL GAME INTEGRATION REVIEW

```
You are doing a final review of the complete GBA supermarket game across all
5 phases. Stack: C, devkitARM, libtonc, Mode 0.

Review checklist:

MEMORY:
- Estimate total IWRAM usage. Must be under 32KB.
- Estimate total EWRAM usage. Must be under 256KB.
- Estimate SaveSlot size × 3. Must be under 32KB total.
- Confirm no VRAM region overlap between charblocks and screenblocks.

CORRECTNESS:
- All prices stored as uint16 cents. No float anywhere.
- No malloc anywhere. All arrays fixed and pre-allocated.
- SRAM writes are 8-bit only.
- OAM entries: player(0-1), NPCs(2-13), employees(14-29), cursor(60),
  float effect(61). Total max ~62 used of 128 available.
- g_cash treated as signed in Phase 3+ for debt display.
- Prestige condition checks all 3 gates (rep+day+cash).

GAMEPLAY:
- Demo win condition reachable with $50 start, 3 products, 1 NPC.
- Phase 5 win requires day 60 minimum — pacing roughly 1 phase per 12 days.
- Reputation 0 = soft lock (no customers) — confirm recovery path exists
  (player can lower prices to attract bargain hunters even at rep 0... but
  rep 0 means 0 spawns. Flag as design issue if no recovery exists).

For each issue: severity (BLOCKER/WARNING/NOTE), file, description, fix.
Output final PASS or FAIL for gold master readiness.
```

-----

## PROMPT USAGE NOTES

- Run prompts in order. Each builds on the last.
- Every REVIEW prompt should receive the code from the matching build prompt.
- COORDINATOR prompts produce no code — they produce a plan. Verify the plan
  matches this document before proceeding.
- FINAL review prompts for each phase must PASS before starting the next phase.
- If a FINAL review returns FAIL, fix flagged issues then re-run FINAL only.
- Paste the relevant section of `supermarket_gba_gdd.md` into any prompt that
  needs spec verification — the GDD is the source of truth.