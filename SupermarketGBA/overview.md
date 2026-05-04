# Supermarket Simulator GBA — Game Design Document

## Overview
**Genre:** Top-down 2D Simulation / Tycoon  
**Platform:** Game Boy Advance — 240×160px, 16-bit color  
**Perspective:** Top-down tile-based (Pokémon/Harvest Moon style)  
**Save:** Battery-backed SRAM, 3 slots  
**Core Loop:** Order Stock → Stock Shelves → Serve Customers → Earn Money → Expand

---

## GBA Technical Constraints

| Constraint | Spec | Solution |
|---|---|---|
| Resolution | 240×160px | 16×16 meta-tiles, 15×10 grid |
| CPU | 16.78MHz ARM7 | No floats; fixed-point & integer math |
| IWRAM | 32KB fast RAM | Active game state only |
| EWRAM | 256KB slow RAM | Maps, catalog, ledger |
| SRAM | 32KB save | 3 slots × ~10KB |
| OAM Sprites | 128 max | Player=2, each NPC=2, cart=1 |
| BG Layers | 4 (Mode 0) | See layer plan below |
| Palettes | 16 BG + 16 SPR (16-color each) | Assigned per category |
| Audio | 4 channels | Ch1-2=music, Ch3=SFX, Ch4=ambient |
| No dynamic alloc | Manual memory only | All arrays pre-allocated at fixed size |

---

## Tile System

### Meta-Tile Grid
- **Tile size:** 16×16px (4× 8×8 hardware tiles)
- **Screen grid:** 15 wide × 10 tall
- **HUD:** Top 10px on BG0 (fixed, no scroll)
- **Play area:** 240×150px effective

### BG Layer Assignments
| Layer | Use | Scrolls |
|---|---|---|
| BG0 | HUD overlay | Fixed |
| BG1 | Foreground (shelf fronts, counter tops) | With camera |
| BG2 | Main floor & walls | With camera |
| BG3 | Background deco (ceiling, posters) | With camera |

### Tile ID Ranges
| Range | Category |
|---|---|
| 000–015 | Floor (linoleum variants, grout, wear) |
| 016–031 | Walls (back, side, corners, baseboard) |
| 032–039 | Shelf — empty state |
| 040–047 | Shelf — stocked state |
| 048–063 | Checkout counter & register |
| 064–079 | Door & entrance |
| 080–095 | Backroom (concrete, racks, boxes) |
| 096–111 | Props & deco (signs, plants, clock) |
| 112–127 | Collision-only / invisible triggers |
| 128–191 | Reserved for expansion |
| 192–255 | Animated tiles (flickering light, price tag spin) |

### Collision Flags (1 byte per tile ID)
| Bit | Meaning |
|---|---|
| 0 | Solid (impassable) |
| 1 | Interact zone (A button) |
| 2 | Shelf slot (linked to product data) |
| 3 | Register zone (triggers checkout) |
| 4 | NPC spawn point |
| 5 | NPC exit trigger |
| 6 | Backroom zone (player restocks) |
| 7 | Reserved |

### Shelf Tile States
Each shelf has a stocked and empty tile ID pair. At runtime, when `stock_on_shelf == 0` the engine swaps to the empty tile ID. Phase 2 adds a low-stock variant.

---

## Demo Store Map (15×10)

```
     0    1    2    3    4    5    6    7    8    9   10   11   12   13   14
0  [WC] [WT] [WT] [WT] [WT] [WT] [WT] [WT] [WT] [WT] [WT] [WT] [WT] [WT] [WC]
1  [WL] [PO] [PO] [SH] [SH] [SH] [FL] [FL] [SH] [SH] [SH] [PO] [PO] [PO] [WR]
2  [WL] [FL] [FL] [S1] [S1] [S1] [FL] [FL] [S2] [S2] [S2] [FL] [FL] [FL] [WR]
3  [WL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [WR]
4  [WL] [FL] [FL] [SH] [SH] [SH] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [WR]
5  [WL] [FL] [FL] [S3] [S3] [S3] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [WR]
6  [WL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [FL] [WR]
7  [WL] [FL] [FL] [FL] [FL] [FL] [FL] [CT] [CT] [CT] [CT] [CT] [FL] [FL] [WR]
8  [WL] [FL] [FL] [FL] [FL] [FL] [FL] [RG] [FL] [FL] [FL] [FL] [FL] [FL] [WR]
9  [WC] [FL] [FL] [FL] [FL] [FL] [DM] [DR] [DR] [DM] [FL] [FL] [FL] [FL] [WC]
```

| Code | Tile | Solid | Interact |
|---|---|---|---|
| WC/WT/WL/WR | Wall corners/edges | ✅ | ❌ |
| FL | Floor | ❌ | ❌ |
| PO | Poster/wall deco | ✅ | ❌ |
| SH | Shelf header (top row) | ✅ | ❌ |
| S1 | Shelf 1 — Bread & Milk | ✅ | ✅ (from row 3) |
| S2 | Shelf 2 — Apples | ✅ | ✅ (from row 3) |
| S3 | Shelf 3 — Expansion (demo: empty) | ✅ | ✅ (from row 6) |
| CT | Checkout counter | ✅ | ❌ |
| RG | Register | ✅ | ✅ |
| DM | Doormat | ❌ | ❌ |
| DR | Door (spawn/exit trigger) | ❌ | ✅ |

**Player start:** (6, 6) — **NPC spawn/exit:** (7–8, 9)

---

## Sprites

### Player
- **Size:** 16×24px — 2 OAM slots (16×16 + 16×8) or single 16×32
- **Palette:** SPR PAL 0 (16 colors)
- **Total frames:** 12 (4 directions × 3 walk frames)

| State | Frames |
|---|---|
| Idle (face down) | 1 |
| Walk (each direction) | 3 |
| Interact / reach | 2 |
| Ring up (at register) | 2 |

### NPC Shoppers
- **Size:** 16×24px — 2 OAM slots per NPC
- **Variants:** 4 (palette swap — blue/red/green/yellow shirt)
- **Frames:** 12 shared across all variants (palette swap only)
- **Demo max:** 1 NPC → Phase 2 max: 4 NPCs

### Shopping Cart
- **Size:** 16×16px — 1 OAM slot per NPC
- **States:** Empty, 1 item, 2+ items (3 frames)
- **Offset:** +8px right of NPC sprite

### Sale Effect
- **Size:** 32×8px — 1 OAM slot (temporary)
- `+$X.XX` floats up over 30 frames then fades

---

## Sprite Sheet
One 128×256px PNG (16-color palettes per sprite set).

```
Rows 0–1   Player walk frames (all directions)
Rows 2–3   NPC base frames (all directions)
Rows 4–5   Shopping cart states + sale effect
Rows 6+    Reserved (employees, Phase 3)
```

---

## Palette Assignments

| Slot | Use |
|---|---|
| BG PAL 0 | Floor (cream, gray grout, worn brown) |
| BG PAL 1 | Walls (off-white, beige baseboard, shadow) |
| BG PAL 2 | Shelves (metal gray, wood brown, price tag white) |
| BG PAL 3 | Counter & register (dark gray, black, silver) |
| BG PAL 4 | Props & deco (sign red, plant green) |
| BG PAL 5 | Backroom (concrete, box tan, rack metal) |
| BG PAL 6–7 | Reserved |
| SPR PAL 0 | Player |
| SPR PAL 1–4 | NPC variants (blue/red/green/yellow) |
| SPR PAL 5 | Cart + UI effect sprites |
| SPR PAL 6–7 | Reserved (employees, Phase 3) |

---

## Tileset Sheet Layout (128×256px)
```
Row  0–1   Floor variants (IDs 000–015)
Row  2–3   Wall tiles (016–031)
Row  4–5   Shelf empty (032–039)
Row  6–7   Shelf stocked (040–047)
Row  8–9   Checkout counter (048–063)
Row 10–11  Door & entrance (064–079)
Row 12–13  Backroom (080–095)
Row 14–15  Props & deco (096–111)
Row 16–17  Collision-only tiles (112–127)
Row 18–31  Expansion / future (128–255)
```

---

## HUD Layout (BG0, fixed, top 10px)

```
┌────────────────────────────────────────────────────┐
│ $ 0 4 7 . 2 3  │  📦 1 2  │  DAY 1  │  2:30 PM   │
│  cols 0–9      │ cols10–16 │cols17–22│ cols 23–29  │
└────────────────────────────────────────────────────┘
```
- Font: 8×8 tile-based glyphs, max ~28 chars per line
- All dollar values: `$XXX.XX` format (stored as cents)
- Product names: 12 char max
- HUD background: solid dark bar (single color tile, repeated)

---

## Player Systems

### Movement
- 4-directional, tile-snapped (no diagonal)
- Speed: 1 tile per input with 2-frame walk cycle
- Collision: tile flag bit 0 check per step

### Controls
| Input | Action |
|---|---|
| D-Pad | Move |
| A | Interact (context-sensitive) |
| B | Cancel / back |
| START | Pause menu |
| SELECT | Inventory / ledger screen |

### Player Actions
| Action | Condition |
|---|---|
| Stock shelf | Facing shelf tile, backroom stock > 0 |
| Ring up customer | At register, customer in queue |
| Place order | In inventory menu, sufficient cash |
| Check prices | In inventory menu (Phase 2) |
| Hire employee | In menu, sufficient cash (Phase 3) |

### Backroom Buffer
Virtual stockroom — not a carried inventory. Ordered items land here. Player walks to shelf → presses A → units transfer from backroom to shelf. Demo capacity: 30 units total.

---

## NPC Shopper Systems

### State Machine
```
SPAWN (door) → GENERATE LIST → NAVIGATE to shelf →
  [item found]  → PICK UP → next item or CHECKOUT
  [shelf empty] → FRUSTRATED (cross off list) → next item or CHECKOUT
QUEUE at register → WAIT → PLAYER RINGS UP → PAY → EXIT
```

### Demo Shopper Attributes
| Attribute | Value |
|---|---|
| Shopping list size | 1–3 items (random from available products) |
| Patience meter | Hidden timer; abandons queue if exceeded |
| Spawn rate | 1 per 45–60 real seconds |
| Walk speed | 1 tile per 0.4s (slower than player) |

### Demo Pathfinding
Hardcoded waypoint routes per destination (Shelf A, Shelf B, Register). No A* needed for demo — NPCs follow scripted tile paths with collision checks. Phase 2 introduces simple A* on the tile grid.

### Phase 2 Shopper Personality Types
| Type | Behavior |
|---|---|
| Bargain Hunter | Skips items priced above market rate |
| Loyal Regular | Buys even at slightly high prices |
| Impulse Buyer | Randomly adds extra items |
| Rushed Shopper | Short patience, small list |
| Big Family | Long list, buys in quantity |

### Satisfaction & Reputation (Phase 2)
Each shopper generates a satisfaction score (items found + wait time + price fairness). Aggregate feeds **Store Reputation** stat. Higher reputation → more spawns per day → more revenue.

---

## Product & Inventory Systems

### Product Data Structure
```c
typedef struct {
  uint8  id;
  char   name[13];       // 12 chars + null
  uint8  category;       // enum: Produce, Dairy, Bakery, etc.
  uint16 shelf_price;    // cents (e.g. 149 = $1.49)
  uint16 cost_price;     // what player pays to order
  uint16 market_price;   // fluctuates Phase 2+
  uint8  stock_on_shelf;
  uint8  shelf_capacity;
  uint8  stock_in_back;
  uint8  units_sold_today;
} Product;
```

### Demo Products
| Product | Cost | Shelf Price | Shelf Capacity |
|---|---|---|---|
| Bread | $0.80 | $1.49 | 6 |
| Milk | $1.10 | $2.29 | 4 |
| Apples | $0.50 | $0.99 | 8 |

### Ordering System
**Demo:** Instant delivery — player opens inventory, selects product + quantity, cost deducted, items appear in backroom immediately.  
**Phase 2:** Orders placed before in-game cutoff (6 PM) arrive next morning. Late orders delayed one extra day. Introduces cash-flow planning.

### Department Categories (Phase 3+)
Produce, Dairy, Bakery, Meat & Seafood, Frozen, Dry Goods, Beverages, Health & Beauty

---

## Economy & Pricing Systems

### Demo Economy
- **Starting cash:** $50.00
- **Expenses (demo):** Restock orders only
- **Transaction flow:** Items scanned → total shown → A to confirm → cash added → float animation

### Pricing Controls (Phase 2)
- Player sets shelf price per product
- **Floor:** 10% above cost (no selling at a loss)
- **Ceiling:** 200% of market price (above triggers shopper backlash)
- Changes take effect immediately on shelf label

### Market Price Fluctuation (Phase 2)
Market price shifts slightly each in-game day, influenced by random supply events (e.g., "Wheat shortage — Bread costs more"). Player absorbs cost or raises shelf price. Core strategic tension of the tycoon loop.

### Daily Financial Ledger (Phase 2)
Tracks: Revenue, Cost of Goods, Gross Profit, Running Balance, Units Sold per Product.

### Expense Categories by Phase
| Expense | Phase |
|---|---|
| Restock orders | Demo |
| Store expansion | 3 |
| Employee wages (daily) | 3 |
| Shelf purchases | 3 |
| Equipment (coolers, display cases) | 4 |
| Rent / lease | 4 |

---

## Store Layout & Progression

### Expansion Phases
| Phase | Size | Unlocks |
|---|---|---|
| Demo | 1 room, 2 shelves | Core loop |
| 2 | 1 room, 5 shelves | Pricing, 10–15 products, 4 NPCs |
| 3 | 2 rooms (camera scroll) | Employees, departments, shelf buying |
| 4 | 3 rooms + backroom | Full logistics, delivery trucks, rent |
| 5 | Full supermarket | Multi-register, deli, bakery, pharmacy |

### Phase 3 Camera Scrolling
Map expands beyond 1 screen. GBA hardware tile scrolling via BG scroll registers. Camera follows player. Tile maps stored in VRAM background layers.

---

## UI Screens

### Inventory / Order Screen (SELECT)
Full-screen BG layer switch. Shows: product name, shelf stock, backroom stock, order quantity selector, cost preview, confirm.

### Checkout UI
Item list scrolls across screen, running total shown. A confirms each item. Final total → A to collect. `+$X.XX` float animation.

### End of Day Summary (Phase 2)
Revenue / Expenses / Profit / Customers Served / Items Sold → transitions to next morning.

### Pause Menu (START)
Save game, settings, quit to title.

---

## Data Architecture

### Memory Layout
| Region | Size | Contents |
|---|---|---|
| IWRAM | 32KB | Active game state, NPC positions, player data |
| EWRAM | 256KB | Map data, product catalog, ledger history |
| SRAM | 32KB | 3 save slots (~10KB each) |

### Runtime Game State
```c
typedef struct {
  uint8  player_x, player_y;
  uint8  player_dir;         // enum: UP/DOWN/LEFT/RIGHT
  uint16 current_time;       // minutes since store open
  NPC    active_npcs[8];
  Shelf  shelves[32];
  NPC*   register_queue[4];
  Order  pending_orders[8];
  Ledger daily_ledger;
} GameState;
```

### Save Slot Structure
```c
typedef struct {
  uint16 day_number;
  uint32 cash_balance;       // cents
  uint8  store_level;
  uint8  reputation;
  uint32 total_revenue;      // cents
  char   store_name[17];     // 16 chars + null
  ProductSave products[32];
  EmployeeSave employees[8]; // Phase 3+
} SaveSlot;
```

---

## Demo Scope Checklist

🟢 = In demo | 🔴 = Post-demo

| System | Status | Notes |
|---|---|---|
| Player movement (4-dir) | 🟢 | Tile-snapped |
| Single-screen store | 🟢 | Fixed map, no scroll |
| 3 products | 🟢 | Bread, Milk, Apples |
| 2 shelves + 1 register | 🟢 | Hardcoded positions |
| 1 NPC shopper | 🟢 | Waypoint pathfinding |
| Manual checkout | 🟢 | Player walks to register |
| Cash balance HUD | 🟢 | Top bar |
| Backroom restock | 🟢 | A at shelf when empty |
| Day timer (store open/close) | 🟢 | Timer-based |
| Basic save (end of day) | 🟢 | SRAM slot 0 |
| Pricing controls | 🔴 | Phase 2 |
| Multiple NPCs (4) | 🔴 | Phase 2 |
| Market price fluctuation | 🔴 | Phase 2 |
| Delayed delivery orders | 🔴 | Phase 2 |
| Store expansion + scroll | 🔴 | Phase 3 |
| Employees | 🔴 | Phase 3 |

**Demo win condition:** Survive 3 in-game days, serve 5+ customers, don't go broke.

---

## Phased Development Roadmap

### Phase 1 — Demo
Tile engine, player movement, collision, static 1-screen map, 3 products, 2 shelves, 1 NPC (waypoints), manual checkout, backroom restock, HUD, day timer, basic SRAM save.

### Phase 2 — Core Tycoon Loop
Pricing controls, market price fluctuation, delayed delivery, 4 NPCs, shopper patience + satisfaction, end-of-day summary, 10–15 products, store reputation.

### Phase 3 — Growth & Delegation
2-room store with camera scroll, employee system (hire/assign/pay), department organization, shelf purchasing, employee stocking AI, up to 20 products.

### Phase 4 — Full Simulation
Delivery truck logistics, rent/overhead, equipment purchases, seasonal events, competitor NPC store, 40+ products.

### Phase 5 — Endgame
Multiple registers, specialty departments (deli, bakery, pharmacy), store remodel system, 50+ products, prestige "Grand Opening" win condition.

---

## Planned Deep-Dive Documents
- [ ] NPC Pathfinding — waypoint system + A* upgrade path
- [ ] Product Catalog — all 50+ products with pricing data
- [ ] Economy Balancing — day-by-day cash flow model
- [ ] Employee AI — behavior trees for hired staff
- [ ] Audio Design — music tracks, SFX list, channel assignments
- [ ] Code Architecture — C file structure (devkitARM + libtonc)