# Butano

[Butano](https://github.com/GValiente/butano) is a modern C++ (C++23)
high-level GBA engine. It manages VRAM/OAM/DMA/backgrounds/sprites
through its own runtime instead of leaving raw hardware-register
pointer arithmetic to game code — which is precisely the category of
bug documented in `footguns.md`. It's the chosen stack for new games in
this repo going forward, in place of continuing with raw-register C or
libtonc.

Tradeoffs, for the record: it's C++ (not C), needs a modern compiler
with C++23 support, has its own asset-conversion pipeline (Python
scripts convert images/audio at build time), and is a bigger, more
opinionated dependency than "a few headers and a linker script." The
payoff is that whole classes of hardware footguns become the engine's
problem instead of every game's problem.

## It's vendored at `external/butano`

Added as a git submodule (`.gitmodules` → `external/butano` →
`https://github.com/GValiente/butano.git`). This session's own GitHub
scope couldn't add or clone the repo directly (see "history" below for
why, kept for context) — it was vendored from a local machine with real
GitHub access and pushed, then merged in here. Once the pointer commit
exists, `git submodule update --init external/butano` fetches the
actual source; that specific command *did* succeed from within this
session even though generic `github.com` browsing/cloning attempts
earlier in the session did not (git's smart-HTTP clone protocol appears
to go through a different, less-restricted path than the web/API
endpoints this session's proxy blocks — not something to rely on, just
what was observed).

## Building it requires devkitARM or Wonderful Toolchain — neither reachable from this sandbox

Confirmed directly from the vendored source
(`external/butano/butano/butano.mak`): the build auto-detects
`$(DEVKITARM)` or `$(WONDERFUL_TOOLCHAIN)` and **errors if neither is
set** — there is no plain-`arm-none-eabi-gcc` fallback like the hand-rolled
ROMs `gba-verify`'s test fixtures use. Specifically, the devkitARM path
(`butano_dka.mak`) needs, beyond the compiler itself:
- `include $(DEVKITARM)/gba_rules` (devkitPro's own build-rule
  makefile)
- `-specs=gba.specs` / `-specs=gba_mb.specs` — GCC spec files bundled
  with devkitARM's *patched* GCC build, not present in upstream/Ubuntu
  `gcc-arm-none-eabi`
- the `grit` (graphics), `mmutil` (Maxmod audio), and `gbafix` (ROM
  header) binary tools devkitPro ships alongside the compiler

None of these are obtainable here: `apt.devkitpro.org` is blocked (see
`toolchain-setup.md`), and `wonderful.asie.pl`/
`toolchain.wonderful.asie.pl` (Wonderful Toolchain's own package
server) are equally blocked by this environment's network policy.

**Practical conclusion:** Butano games in this repo build on a machine
that actually has devkitARM installed — which is already true of
whoever built `BrickBreak`/`HollowShore` locally, since those also
require `DEVKITARM` — not inside this sandboxed session. `gba-verify`'s
headless mGBA harness still applies once a `.gba` exists, regardless of
where it was compiled; it doesn't care how the ROM was built.

## Starting a new game on Butano

Confirmed from `external/butano/butano/include/documentation/bn_documentation_aa_getting_started.h`
(the real getting-started doc, read from the vendored source rather
than the getting-started website, which this session can't reach):

1. Copy `external/butano/template/` to the new project's location.
2. In its `Makefile`, point `LIBBUTANO` at the vendored engine —
   `LIBBUTANO := ../../external/butano/butano` (adjust the relative
   path to wherever the new project directory ends up relative to
   `external/butano/butano`).
3. Optionally set `ROMTITLE`/`ROMCODE` and other template `Makefile`
   variables for the new game.
4. `cd` into the new project directory and `make -jN` (from a machine
   with `DEVKITARM` set, per the build requirement above) — should
   produce a `.gba` file if everything's wired correctly.
5. Run it through `gba-verify` before trusting it.

The template already includes `src/main.cpp`, and `graphics`/`audio`/
`dmg_audio`/`include` directories with `.gitignore`s ready for assets —
don't hand-roll a `main.cpp`/`Makefile` from scratch, start from this
template every time.

## Reference

- Repo: `external/butano` (vendored) / `github.com/GValiente/butano`
  (upstream)
- Official examples: `external/butano/examples/` — build one (e.g.
  `sprites`) as a sanity check before starting a new game, per the
  getting-started doc above.
- Two full example games ship with the engine at `external/butano/games/`
  — worth a look for real-world structure before designing a new one.
- `awesome-gbadev` (github.com/gbadev-org/awesome-gbadev) lists Butano
  alongside other engines/libraries/tools if the stack decision ever
  needs revisiting for a specific game.
