# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository overview

This is not one project — it's a monorepo of independent Game Boy Advance
homebrew games plus dev tooling shared across them:

- `BrickBreak/` — small, self-contained Breakout clone. Raw hardware register
  access, no library beyond devkitARM's boilerplate. Mode 3 (16-bit bitmap).
- `SupermarketGBA/` — top-down tycoon/sim ("Supermarket Simulator GBA"), built
  on **libtonc**, Mode 0 tile engine. `SupermarketGBA/overview.md` is the full
  GDD (tile layout, sprite budget, economy/save-data structs, phased roadmap)
  — read it before touching this project's game logic.
- `HollowShore/` — the largest and most ambitious project: a Zelda/Stardew-
  hybrid RPG (sanctums/bosses, combat, crafting, farming, weather, building,
  inventory, save system). Raw hardware register access, no library, ~60
  source files. `HollowShore/Overall - BUILD_PLAN.md` is a phased agent task
  list; note it references a repo-root design doc (`Hollow-Shore-Plan`) that
  no longer exists in this repo (deleted in a past reorg commit) — treat the
  build plan's task list as historical context, not a currently-authoritative
  spec on its own.
- `external/butano/` — the [Butano](https://github.com/GValiente/butano) C++
  engine, vendored as a git submodule. This is the chosen stack for **new**
  games going forward (not a rewrite of the three existing projects) — see
  `.claude/skills/gba-dev/reference/butano.md` for why and how to start a new
  project from `external/butano/template/`.

Each game project builds independently; there is no root-level build. Do not
assume changes to one project affect another.

## Build commands

All three existing projects require devkitPro's devkitARM toolchain
(`DEVKITPRO`/`DEVKITARM`/`LIBGBA` env vars) and **will not build with a plain
`arm-none-eabi-gcc`** — see the "does NOT currently build" section of
`.claude/skills/gba-dev/reference/toolchain-setup.md` for exactly why (missing
`gba_rules`, `libgba`, `libtonc`, `gbafix`, or — for Butano — devkitARM's
patched GCC `.specs` files). In a Claude Code web session, devkitPro's package
server is network-blocked, so none of these currently build in-session; they
build on a machine with devkitARM actually installed.

```bash
# BrickBreak
cd BrickBreak && make        # -> BrickBreakerGBA.gba
make clean

# SupermarketGBA (libtonc)
cd SupermarketGBA && make    # -> supermarket.gba
make clean

# HollowShore
cd HollowShore && make       # -> HollowShore.gba
make clean

# A new Butano-based project (once scaffolded from external/butano/template/)
cd <project-dir> && make -j$(nproc)
```

## Verification workflow

There is no conventional unit test suite. A clean compile proves nothing on
this platform — GBA hardware bugs (bad VRAM/OAM/palette writes, invalid DMA,
etc.) are silent at compile time and often silent at runtime too, unless you
go looking. Two skills exist specifically for this:

- **`.claude/skills/gba-dev`** — read `reference/footguns.md` before writing
  or reviewing any code that touches VRAM, OAM, palette RAM, DMA, or
  interrupts directly. Most GBA bugs in this repo trace back to one line on
  that list (see `reference/hollowshore-notes.md` for a live example: the
  checked-in `HollowShore/ConsoleLog` shows a continuous, never-caught
  `Bad memory Store8` OAM-corruption error from a past test session).
- **`.claude/skills/gba-verify`** — after building a `.gba`, run it headlessly
  through mGBA to actually confirm it boots and runs cleanly before calling a
  task done:
  ```bash
  python3 .claude/skills/gba-verify/verify_rom.py --rom path/to/Game.gba \
    --frames 600 --screenshot-at 1,60,300,600 --out-dir /tmp/verify-out
  ```
  Exits non-zero if mGBA logs a FATAL/ERROR/GAME_ERROR-level line during the
  run. See that skill's `SKILL.md` for what it does and doesn't catch (e.g. it
  catches invalid 8-bit OAM writes via mGBA's own diagnostics, but *not*
  8-bit VRAM/Palette writes, which hardware silently mirrors instead of
  rejecting — check memory state directly for those).

## Toolchain / environment notes

`.claude/hooks/session-start.sh` installs the ARM cross-compiler and headless
mGBA automatically via apt at session start (idempotent). `.claude/skills/gba-dev/reference/toolchain-setup.md`
has the exact verified commands and known gotchas (e.g. a `libva2` mirror
pin needed for mGBA's apt dependencies). Read it before assuming a toolchain
problem is new — it's probably already documented there.

## Repo quirks worth knowing about

- The root `README.md` is actually `BrickBreak`'s README (a leftover from a
  past repo reorg), not a repo-wide overview — don't treat it as describing
  the whole monorepo.
- `RunLater.py` is a local Windows automation script (window-focus + delayed
  keystroke) unrelated to any game's build or runtime.
