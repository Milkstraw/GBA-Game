# Hollow Shore — Agent Build Plan

## Before You Start

Read `/Hollow-Shore-Plan` at the repository root. That file is the full game design
document and is the authoritative source for all gameplay behaviour. This file only
assigns implementation tasks — it does not restate the design.

The source tree lives in `HollowShore/`. Build with devkitPro/devkitARM:
`make` inside `HollowShore/` → produces `HollowShore.gba`.

Each task is self-contained. Do tasks in order unless "Depends on" says otherwise.
When you finish a task, commit with a message referencing the task number (e.g.
`Task 0.1 — GBA boilerplate`), then push to the active branch.

---

## Phase 0 — Foundation

### Task 0.1 — GBA Boilerplate  ← START HERE
**Goal:** Compile a ROM that boots, runs the main loop, and reads input.

**Files to implement:**
- `HollowShore/Makefile` — devkitPro build; TARGET=HollowShore; SOURCES lists all subdirs
- `HollowShore/source/core/types.h` — u8/u16/u32/s8/s16/s32 typedefs + bool
- `HollowShore/source/gba.h` — REG_DISPCNT, REG_VCOUNT, REG_KEYINPUT, mode/BG enable
  bits, vsync(), KEY_* bitmasks
- `HollowShore/source/core/input.h/.c` — key_poll(), key_held(mask), key_pressed(mask)
- `HollowShore/source/main.c` — game-loop stub: vsync → poll input → toggle a solid
  palette colour on A press

**Done when:** `make` in `HollowShore/` produces `HollowShore.gba` with zero warnings.

---

## Phase 1 — Display

### Task 1.1 — Tile Engine Setup
**Goal:** Render a static background (BG0, Mode 0) with a tiny 2-tile tileset.

**Files to implement:**
- `HollowShore/source/graphics/renderer.h/.c` — init_renderer(), load_bg_tiles(),
  set_bg_tile(x, y, tile_id); configure BG0 as 32×32 tilemap, 4bpp
- `HollowShore/source/graphics/palette.h/.c` — load_bg_palette(), load_sprite_palette()
- Hardcode a 2-tile (16×16 px, 4bpp) tileset (grass + dirt) as a u32 array in renderer.c

**Done when:** Screen shows a checkerboard of two tile colours with no flicker.

**Depends on:** Task 0.1

### Task 1.2 — Sprite System
**Goal:** Display a 16×16 player sprite at screen centre using OAM.

**Files to implement:**
- `HollowShore/source/graphics/sprites.h/.c` — oam_init(), sprite_set(id, x, y,
  tile, pal), sprite_hide(id), copy_oam() via DMA
- Minimal 16×16 placeholder sprite (solid block with a dot) as a u32 array in sprites.c

**Done when:** Sprite is visible at screen centre and does not flicker between frames.

**Depends on:** Task 1.1

---

## Phase 2 — Player Movement

### Task 2.1 — Player Entity & D-Pad Movement
**Goal:** Sprite moves with D-pad at 4 px/frame; B held = sprint at 8 px/frame.

**Files to implement:**
- `HollowShore/source/player/player.h/.c` — Player struct (x, y, facing, state),
  player_init(), player_update(keys), player_draw()
- `HollowShore/source/player/stats.h/.c` — PlayerStats (hp, max_hp, stamina,
  max_stamina, hunger), stats_init()

**Done when:** Sprite moves in all 4 directions; sprint doubles speed.

**Depends on:** Task 1.2

### Task 2.2 — World Coordinates & Camera Scroll
**Goal:** World is 160×160 tiles (2560×2560 px). Camera follows player via
REG_BG0HOFS/VOFS. Player is clamped to screen centre except at map edges.

**Files to implement:**
- `HollowShore/source/world/section.h/.c` — Section struct; SECTION_TILES_W=160,
  SECTION_TILES_H=160, TILE_SIZE=16; camera_x/camera_y
- Update player.c to use world coords

**Done when:** Walking toward any edge scrolls the background; movement stops at
world boundary.

**Depends on:** Task 2.1

---

## Phase 3 — World & Sections

### Task 3.1 — Section Tile Map Data
**Goal:** Define tile types and hard-code Section 5 (spawn) with 5 tile variants.

**Files to implement:**
- `HollowShore/source/world/tile.h` — TileType enum (TILE_GRASS, TILE_DIRT,
  TILE_WATER, TILE_TREE, TILE_ROCK, …); TileDef (passable, resource_type)
- `HollowShore/source/world/tile.c` — tile_defs[] lookup table
- `HollowShore/source/world/maps/world_a.h/.c` — section5_tiles[160*160] u8 array
  (hand-authored; simple clearing with trees and rocks around the edges)

**Done when:** section5_tiles compiles and can be indexed by (x + y*160).

**Depends on:** Task 2.2

### Task 3.2 — Section Rendering
**Goal:** Render section tile map to BG0; only visible 30×20 tiles updated each frame.

**Files to implement:**
- Update renderer.c — section_render_visible(section, cam_x, cam_y)
- Expand tileset to cover all 5 tile types from Task 3.1

**Done when:** Player walks across varied terrain (grass, trees, rock, water) with
seamless scrolling.

**Depends on:** Task 3.1

### Task 3.3 — Section Transitions
**Goal:** Walking off a section edge triggers fade-to-black, loads adjacent section,
player appears at matching edge. Out-of-bounds edges are impassable.

**Files to implement:**
- `HollowShore/source/world/transition.h/.c` — transition_check(), transition_update(),
  transition_fade_out(), transition_fade_in()
- Update section.c — section_load(id), section_save_state(id),
  section_id_from_direction(current_id, dir)

**Done when:** Walking off the right edge of Section 5 fades to black and loads
Section 6; player appears at Section 6's left edge.

**Depends on:** Task 3.2

---

## Phase 4 — Core Survival Systems

### Task 4.1 — Day/Night Cycle
**Goal:** In-game clock advances 1 hour per real minute (24-min full day). Sky
colour transitions through dawn/day/dusk/night via BG palette swap.

**Files to implement:**
- `HollowShore/source/core/clock.h/.c` — GameClock (hour, day, frame_counter),
  clock_update(), clock_get_phase() → PHASE_DAWN/DAY/DUSK/NIGHT
- Update palette.c — palette_apply_time_of_day(phase, hour)

**Done when:** Running for 24 real minutes produces a full visible day cycle.

**Depends on:** Task 3.2

### Task 4.2 — HUD (Health / Stamina / Hunger / Hotbar)
**Goal:** BG3 layer shows HP bar, stamina bar, hunger bar, and 8-slot hotbar.

**Files to implement:**
- `HollowShore/source/ui/hud.h/.c` — hud_draw(stats, hotbar), draw_bar(x, y,
  val, max, color)
- `HollowShore/source/ui/text.h/.c` — Minimal 8×8 font, text_draw(x, y, str)

**Done when:** Three labelled bars and hotbar render over the tile background.

**Depends on:** Task 2.1, Task 1.1

### Task 4.3 — Stamina System
**Goal:** Stamina drains on sprint and tool use; regenerates at rest (faster near
bed). Low stamina reduces tool speed; zero stamina disables sprint.

**Files to implement:**
- Update stats.h/.c — stamina drain/regen rates, stamina_update(player, keys,
  near_bed)
- Update player.c — clamp sprint to stamina > 0

**Done when:** Stamina bar visibly depletes on sprint and recovers when idle.

**Depends on:** Task 4.2

### Task 4.4 — Hunger System
**Goal:** Hunger drains over time (faster during stamina activity). Stages: Full →
Satisfied → Hungry → Starving. Starving drains HP and blocks stamina regen.

**Files to implement:**
- Update stats.h/.c — hunger value, hunger_update(stats, is_active), hunger_stage()

**Done when:** Idle play drains through all four stages; HP drains at Starving.

**Depends on:** Task 4.3

### Task 4.5 — Inventory System
**Goal:** 16 base slots + 8 hotbar slots. Select opens inventory. L/R cycles
hotbar cursor. Items stack. Select+A auto-sorts.

**Files to implement:**
- `HollowShore/source/player/inventory.h/.c` — Item (type, quantity), Inventory
  (slots[24], hotbar_cursor), inventory_add(), inventory_remove(), inventory_sort()
- `HollowShore/source/ui/menus.h/.c` — draw_inventory_screen(inventory)

**Done when:** Inventory screen opens, shows a slot grid, and items added
programmatically appear in correct slots.

**Depends on:** Task 4.2

---

## Phase 5 — Gathering, Crafting & Building

### Task 5.1 — Resource Gathering
**Goal:** A adjacent to a resource tile with the correct tool removes the tile,
sets a respawn timer (3 days), and adds items to inventory.

**Files to implement:**
- `HollowShore/source/systems/crafting.h` — ItemType enum (ITEM_WOOD, ITEM_STONE,
  ITEM_FLINT, ITEM_BRANCH, ITEM_AXE, ITEM_PICKAXE, ITEM_SHOVEL, …)
- Update input.c — face_tile_x/y (tile in the direction player faces)
- Update section.c — section_set_tile(), ResourceNode (respawn_day counter)

**Done when:** Pressing A next to a tree with an axe equipped removes the tree tile
and adds ITEM_WOOD to inventory.

**Depends on:** Task 4.5, Task 3.2

### Task 5.2 — Crafting System
**Goal:** Crafting menu shows auto-unlocked recipes (unlock on first pickup of a
required material). Hands station always available.

**Files to implement:**
- `HollowShore/source/systems/crafting.h/.c` — Recipe struct, recipe_book[],
  crafting_unlock_check(item_type), crafting_execute(recipe_id, inventory)
- Update menus.c — draw_crafting_screen(unlocked_recipes, inventory)

**Done when:** Picking up ITEM_WOOD unlocks the Campfire recipe. Executing it
consumes wood and adds ITEM_CAMPFIRE to inventory.

**Depends on:** Task 5.1

### Task 5.3 — Building System
**Goal:** Place/break structures on the tile grid. Four wall tiers (Wood→Stone→
Brick→Metal). Upgrade in-place. Torches/lanterns have a light radius that
suppresses monster spawns.

**Files to implement:**
- `HollowShore/source/systems/building.h/.c` — Structure (type, tier, x, y),
  placed_structures[] per section, building_place(), building_break(),
  building_upgrade(), lighting_radius(structure)
- Update renderer.c — render_structures(section)

**Done when:** A wood wall can be placed, upgraded to stone, then broken to recover
partial materials.

**Depends on:** Task 5.2

---

## Phase 6 — Farming & Weather

### Task 6.1 — Farming System
**Goal:** Shovel turns grass to tilled soil. Seeds planted, watered, harvested after
season-appropriate growth period. Seasons cycle every 7 in-game days.

**Files to implement:**
- `HollowShore/source/systems/farming.h/.c` — CropTile (seed_type, water_day,
  growth_stage), farming_till(), farming_plant(), farming_water(),
  farming_tick(day), season_from_day(day)
- Add tile types: TILE_TILLED, TILE_CROP_1/2/3

**Done when:** Tilling, planting, watering, and waiting the correct days produces a
harvestable crop tile.

**Depends on:** Task 5.1, Task 4.1

### Task 6.2 — Weather System
**Goal:** Weather changes every 1–3 in-game days. States: Clear, Rain, Storm,
Blizzard (winter only). Rain auto-waters crops. Storm chips structure HP.
Blizzard increases hunger drain.

**Files to implement:**
- `HollowShore/source/systems/weather.h/.c` — WeatherState enum, weather_update(),
  weather_apply_effects(section, stats)
- Update palette.c — weather overlay palette shift

**Done when:** Weather changes after a few days; screen palette reflects the state;
rain waters crops without manual input.

**Depends on:** Task 6.1, Task 4.1

---

## Phase 7 — Combat & Monsters

### Task 7.1 — Basic Combat
**Goal:** A with sword/axe/spear/bow attacks. Melee hits the tile in front. Bow
fires a projectile. Enemies flash on hit; die at 0 HP and drop loot.

**Files to implement:**
- `HollowShore/source/systems/combat.h/.c` — attack_melee(player, section),
  attack_ranged(player, section), Projectile struct, projectile_update()
- `HollowShore/source/systems/monsters.h/.c` — Enemy (type, hp, x, y, state),
  enemy_take_hit(enemy, damage)

**Done when:** Sword swing reduces enemy HP; enemy disappears and drops loot at 0 HP.

**Depends on:** Task 4.5, Task 3.2

### Task 7.2 — Monster AI
**Goal:** Enemies spawn at section edges after dusk and pathfind toward player.
They attack player and adjacent structures. Hard cap: 10 enemies active at once.

**Files to implement:**
- Update monsters.c — enemy_update(enemy, player, section), enemy_spawn(),
  pathfind_step() (simple 8-direction chase or A*)

**Done when:** Enemies appear at night, chase the player, and attack on adjacency.

**Depends on:** Task 7.1, Task 4.1

### Task 7.3 — Monster Escalation & Siege Events
**Goal:** Enemy tier scales by day (1–3 / 4–7 / 8–15 / 15+). Day 15+ spawns
coordinated siege waves every 3rd night.

**Files to implement:**
- Update monsters.c — spawn_tier_from_day(day), siege_event_check(day, night_count),
  spawn_wave(tier, section)

**Done when:** Day 1 only slimes. By Day 15, an armored-goblin wave arrives every
3rd night.

**Depends on:** Task 7.2

---

## Phase 8 — Sanctums

### Task 8.1 — Dungeon Framework
**Goal:** A dungeon (3 floors) loads separately from world sections. Floor tiles use
the same tile engine. Entrance/exit tiles transition between floors.

**Files to implement:**
- `HollowShore/source/sanctums/sanctum.h/.c` — Sanctum (id, floors[3],
  boss_defeated), DungeonFloor struct, sanctum_enter(), sanctum_floor_load(),
  sanctum_exit()

**Done when:** Stepping on a sanctum entrance tile loads Floor 1 of a blank dungeon;
reaching the exit tile loads Floor 2.

**Depends on:** Task 3.3, Task 7.1

### Task 8.2 — Sanctum 1: The Roothold
**Goal:** Floors: pressure-plate puzzles → ambush/chest rooms → Root Warden boss.
Boss drops Verdant Shard (Portal Piece 1) + Bark Steel.

**Files to implement:**
- `HollowShore/source/sanctums/roothold.h/.c` — floor tile data (3 floors),
  root_warden_update(), root_warden_phase()
- Attacks: shockwave lines, Vine Whip minions, root status

**Done when:** All 3 floors completable; Root Warden defeated → Verdant Shard in
inventory.

**Depends on:** Task 8.1

### Task 8.3 — Sanctum 2: The Tidecrypt
**Goal:** Tidal water rises/falls on a 30-second real-time timer. Swimmer enemies.
Boss: Drowned Warden (tidal wave sweep, Tide Crawler spawns).

**Files to implement:**
- `HollowShore/source/sanctums/tidecrypt.h/.c` — tidal_timer, tide_state,
  tide_update(), drowned_warden_update()

**Done when:** Water level visibly changes; boss defeated → Tide Shard.

**Depends on:** Task 8.1

### Task 8.4 — Sanctum 3: The Frostspire
**Goal:** Floors ascend. Ice tiles apply momentum sliding. Frozen-enemy blocks usable
as cover. Boss: Glacial Warden (splits into two forms at 50% HP).

**Files to implement:**
- `HollowShore/source/sanctums/frostspire.h/.c` — ice_momentum(), 
  glacial_warden_update(), warden_split_phase()

**Done when:** Player slides on ice; boss splits at half HP; boss defeated → Frost
Shard.

**Depends on:** Task 8.1

### Task 8.5 — Sanctum 4: The Marshveil
**Goal:** Fog of war via GBA hardware window on Floor 1 (torch required). Mimic
enemies on Floor 2. Boss: Bog Specter (invisible, floor-ripple telegraph, poisons).

**Files to implement:**
- `HollowShore/source/sanctums/marshveil.h/.c` — fog_window_update(torch_x, torch_y),
  mimic_enemy_update(), bog_specter_update()

**Done when:** Screen is mostly masked without torch; boss defeated → Veil Shard.

**Depends on:** Task 8.1

### Task 8.6 — Sanctum 5: The Emberdepth
**Goal:** Lava tiles deal damage unless cooled with water flasks or bridged. Iron
Golem enemies need high damage threshold. Boss: Ember Titan (creates permanent lava
pools until cooled).

**Files to implement:**
- `HollowShore/source/sanctums/emberdepth.h/.c` — lava_tile_damage(),
  lava_cool(x, y), golem_enemy_update(), ember_titan_update()

**Done when:** Lava tiles harm player; cooled with water flask; boss defeated →
Ember Shard.

**Depends on:** Task 8.1

### Task 8.7 — Sanctum 6: The Hollow Throne
**Goal:** Entrance gate locked unless all 5 Portal Pieces are held. Corruption zones
drain HP. Echo enemies mirror player movement (delayed). Boss: Hollow Sovereign
(Phase 1: all previous boss types; Phase 2: inverted controls, corruption waves).

**Files to implement:**
- `HollowShore/source/sanctums/hollow_throne.h/.c` — entry_gate_check(inventory),
  corruption_tile_drain(), echo_enemy_update(), hollow_sovereign_update(),
  phase2_inverted_controls()

**Done when:** Entry blocked without all 5 shards; boss defeated → Hollow Keystone
(Portal Piece 6).

**Depends on:** Tasks 8.2–8.6 (all 5 prior shards must exist in codebase)

---

## Phase 9 — Save, Maps & Polish

### Task 9.1 — Save System
**Goal:** 128KB SRAM. Auto-save on sleep (bed + L+R). Saves: section id, player
position, inventory, day count, section tile-change bitfields, boss defeat flags,
season, weather. Pause menu manual save option.

**Files to implement:**
- `HollowShore/source/core/save.h/.c` — SaveData struct, save_write(),
  save_read(), save_validate_checksum()
- Update menus.c — Manual Save menu item

**Done when:** save_write() + save_read() cycle restores exact world state.

**Depends on:** Task 4.1, Task 4.5, Task 3.3, Task 5.3

### Task 9.2 — World Maps A, B & C
**Goal:** All three preset world maps have full tile data for all 9 sections.
New game prompts world selection before spawning in Section 5.

**Files to implement:**
- `HollowShore/source/world/maps/world_a.c` — All 9 section tile arrays, World A
- `HollowShore/source/world/maps/world_b.c` — All 9 section tile arrays, World B
- `HollowShore/source/world/maps/world_c.c` — All 9 section tile arrays, World C
- Update menus.c — World selection screen on new game

**Done when:** Selecting World B at new-game loads the correct Section 5 tile map
for Shattered Isles.

**Depends on:** Task 3.1

### Task 9.3 — Menu Polish & Font
**Goal:** Pause menu (Resume / Save / Options). Inventory tabs
(All/Tools/Materials/Food/Seeds/Equipment). Scrollable crafting list. Full printable
ASCII in the 8×8 font.

**Files to implement:**
- Update menus.c — pause_menu(), inventory_tab_draw(), crafting_list_scroll()
- Update text.c — Full printable ASCII charset

**Done when:** Pause, inventory, and crafting screens are fully navigable with
correct tab filtering and readable text.

**Depends on:** Task 4.5, Task 5.2

### Task 9.4 — Endgame & Credits
**Goal:** Combining all 6 Portal Pieces at the Alchemy Table crafts the Tide Portal.
Placing it in Section 5 triggers a credits scroll (world name + playtime). Post-game
offers sandbox mode (monster escalation continues; portal goal removed).

**Files to implement:**
- Update crafting.c — tide_portal_recipe (requires all 6 shards + biome materials)
- `HollowShore/source/core/endgame.h/.c` — credits_roll(world_name, playtime),
  sandbox_mode_enable()

**Done when:** All 6 shards crafted into a portal and placed in Section 5 triggers
credits. Choosing Stay continues play in sandbox mode.

**Depends on:** Task 8.7, Task 5.2, Task 9.1
