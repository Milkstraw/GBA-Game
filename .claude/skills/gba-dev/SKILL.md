---
name: gba-dev
description: Use when writing, reviewing, or debugging Game Boy Advance homebrew code in this repo (BrickBreak, SupermarketGBA, HollowShore, or a new project) — whether raw-register C, libtonc, or Butano C++. Load before writing GBA hardware-touching code (VRAM, OAM, palette, DMA, interrupts, sprites, backgrounds, save/SRAM) and before reviewing it, not just when something already broke. Trigger on: GBA, Game Boy Advance, Butano, libtonc, tonc, VRAM, OAM, DMA, REG_DISPCNT, mode 0/3/4, sprite, palette, .gba rom, devkitARM.
---

# GBA development in this repo

GBA hardware programming has a small, well-known set of footguns that
don't show up in general C/C++ knowledge and that an LLM will
confidently get wrong unless it deliberately checks against them —
because they're not bugs in your logic, they're undocumented-feeling
quirks of specific memory-mapped hardware. `HollowShore/ConsoleLog` in
this repo is a real example: the game ran while continuously hitting an
8-bit write into OAM, silently corrupting sprite data every frame,
because nothing in the build (compiles clean) or the code (looks like
ordinary C) would tip you off.

This skill's job is to put that knowledge in front of you *before* you
write hardware-touching code, and to pair with `gba-verify` so a "did it
actually run correctly" check happens before any task is called done.

## Read this first, every time

**`reference/footguns.md`** — the condensed cheat sheet of GBA-specific
gotchas (VRAM/OAM/palette write-width rules, DMA, interrupts, fixed-point
math, sprite limits). Read it before writing or reviewing any code that
touches a hardware register or memory-mapped region directly. Most
GBA bugs trace back to one line on that list.

## Stack decision for this repo

**New games should be built on [Butano](https://github.com/GValiente/butano)**, a modern C++ engine that manages VRAM/OAM/DMA safely under the hood instead of leaving raw pointer arithmetic to whoever's writing the code that day. This was chosen deliberately over continuing with raw-register C or libtonc: the existing projects show the raw-register approach compounding bugs at scale (HollowShore has ~60 hand-written source files touching hardware directly), and Butano's abstractions eliminate whole categories of the footguns in `reference/footguns.md` by construction rather than by remembering not to trip them.

- `BrickBreak` (raw registers, small, Mode 3) and `SupermarketGBA`
  (libtonc) are existing projects on the old stack — don't rewrite them
  unless asked, but any *new* hardware-touching code added to them
  should still be checked against `reference/footguns.md`.
- `HollowShore` is the project most in need of the new tooling given its
  size and the corruption already logged; see
  `reference/hollowshore-notes.md` for what the checked-in
  `ConsoleLog` actually shows.
- See `reference/butano.md` for how to actually get Butano into a
  project (it is **not vendored in this repo** — see that file for why
  and what command to run).

## Toolchain

See `reference/toolchain-setup.md` for exact, verified-working install
commands for the ARM cross-compiler and mGBA (both installable from
plain apt in this environment — devkitPro's own server is blocked by
this environment's network policy, but the Ubuntu-packaged
`gcc-arm-none-eabi` produces correct ARMv4T/Thumb-interworking code and
was confirmed to actually boot in mGBA). A SessionStart hook installs
this automatically for new sessions.

**Caveat:** this apt-based toolchain does not currently satisfy
`BrickBreak`, `SupermarketGBA`, or `HollowShore`'s existing Makefiles —
all three hard-require devkitPro's own `gba_rules`/`libgba`/`gbafix`,
which aren't reachable here. It's proven for code that brings its own
crt0/linker script (hand-rolled, or Butano's once vendored). See the
"does NOT currently build" section of `reference/toolchain-setup.md`
before assuming `make` works in one of the existing project folders.

## After writing code

Use the `gba-verify` skill to build (with the project's own Makefile)
and then actually run the ROM headlessly before saying a task is done.
A clean compile proves nothing on this platform — see that skill for
why.
