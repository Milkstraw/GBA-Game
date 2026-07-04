# GBA hardware footguns

Condensed, high-signal list. Every entry here is a class of bug that
compiles cleanly, looks like ordinary C/C++, and silently corrupts
state or does nothing at runtime. When reviewing GBA code, check every
line that touches a hardware address (anything in `0x04000000`–
`0x07FFFFFF`, or a register/pointer derived from one) against this list.

## 1. Write width matters, and the failure mode differs by region

GBA VRAM, OAM, and Palette RAM are 16-bit-wide memory. Writing to them
with an 8-bit (`u8`/`char`) store does **not** do what a naive port of
PC code expects, and the three regions fail differently:

- **OAM** (`0x07000000`–`0x070003FF`, sprite attribute data): 8-bit
  writes are **ignored entirely**. mGBA logs this explicitly —
  `Cannot Store8 to OAM: <addr>` at GAME_ERROR level — which is exactly
  what `gba-verify` catches.
- **VRAM** (`0x06000000`+) and **Palette RAM** (`0x05000000`+): 8-bit
  writes are **not ignored and not logged** — the hardware writes the
  same byte value into *both* the high and low byte of the addressed
  16-bit halfword. If you meant to change one pixel/index and leave its
  neighbor alone, you just silently overwrote the neighbor too. This is
  a data-corruption bug with **no emulator warning**, which is why it's
  more dangerous than the OAM case, not less.

Rule: never write to these three regions through a `u8*`/`char*`. Always
go through a `u16*` (or `u32*` for bulk/DMA transfers), building the
halfword value yourself if you only meant to touch "half" of it (e.g.
`(new_hi << 8) | old_lo`).

## 2. VRAM/OAM/Palette access timing

The CPU can access VRAM, OAM, and Palette RAM at any time, but the
**display controller** is also reading them every scanline while
rendering is active. Large or frequent writes during active display can
tear (visible partway through a frame) or, for OAM specifically, corrupt
sprites if you don't set `H-Blank Interval Free` in `DISPCNT`. Do bulk
updates (background scroll, palette swaps, full OAM buffer flush) during
VBlank (poll `REG_VCOUNT` or wait on the VBlank interrupt), not
scattered through mid-frame game logic.

## 3. DMA gotchas

- Only DMA channels **1 and 2** are meant for audio FIFO buffering;
  don't repurpose them for general transfers if you're also using
  sound.
- **DMA 0 cannot access the cartridge ROM/bus at all** — source must be
  IWRAM/EWRAM. If a DMA-based copy from ROM data silently does nothing,
  check which channel you used.
- An invalid DMA source address doesn't just fail cleanly — a
  subsequent DMA from a *valid* region can end up reading stale state
  from the previous bad configuration. Don't assume "the DMA I set up
  last is irrelevant now" without verifying the current one actually
  completed correctly.

## 4. Interrupt acknowledgment

Acknowledge with `REG_IF = IRQ_X`, never `REG_IF |= IRQ_X`. The
hardware's interrupt-flag register is cleared by *writing a 1 to the
bit*, so `|=` against a register that already has other pending bits
set will spuriously clear interrupts you haven't handled yet if more
than one fires in the same window. This is a real race, not a
theoretical one, once you have VBlank + timer + serial interrupts all
live.

## 5. No floats, no heap

- The ARM7TDMI has no FPU. Floating point compiles, but every operation
  is emulated in software at a real cost, and mixing fixed-point game
  math with floats invites subtle rounding mismatches. Use fixed-point
  (commonly Q8.8: integers shifted left 8 bits, i.e. multiplied by 256)
  for anything performance- or precision-sensitive — positions,
  velocities, angles.
- Don't `malloc`/`new` at runtime. There's no OS, a tiny fixed heap if
  any, and fragmentation on a system this constrained is a real failure
  mode, not a style preference. Use fixed-size static arrays sized for
  the actual maximum (max entities, max inventory slots, etc.).

## 6. Sprite (OBJ) limits

- 128 OAM entries total, shared across every sprite on screen. Budget
  for this explicitly — a "spawn as many as you want" enemy system will
  silently stop rendering (or worse, index past OAM) past entry 128.
- Sprites use either **1D or 2D tile-mapping mode** (a `DISPCNT` bit).
  Getting this wrong doesn't crash, it just shows garbled/wrong tiles
  for any sprite wider or taller than 8x8 — check which mode your
  sprite-drawing code assumes matches what you set in `DISPCNT`.

## 7. SRAM/save writes

The SRAM bus is 8-bit only — this is one of the few places an 8-bit
access is *correct and required*, not a footgun, but writing 16/32-bit
values here (the opposite mistake of everything above) silently
corrupts or drops bytes. Save-data code should be the one place in the
codebase deliberately using `u8*`.

## Why this list, specifically

These are the failure modes that (a) compile without warnings, (b) look
identical to correct code to anyone pattern-matching from general C
experience, and (c) are exactly the kind of thing `HollowShore`'s
checked-in `ConsoleLog` shows happening in practice. See
`hollowshore-notes.md` for the concrete example already in this repo.
