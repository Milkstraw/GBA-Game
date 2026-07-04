---
name: gba-verify
description: Use after building or changing any GBA ROM in this repo (BrickBreak, SupermarketGBA, HollowShore, or a new project) to confirm it actually boots and runs cleanly before calling the task done. Compiles nothing itself — build with the project's own Makefile first — then runs the resulting .gba headlessly in mGBA for a fixed number of frames via the mgba Python bindings, capturing mGBA's own hardware-level diagnostics (invalid VRAM/OAM/palette writes, unimplemented BIOS/register stubs, DMA misuse) and a screenshot. Trigger whenever GBA source changed and a build succeeded — a clean compile does not mean the ROM runs correctly.
---

# GBA headless verify loop

A GBA ROM can compile with zero warnings and still be badly broken at
runtime — bad pointer math into hardware registers doesn't segfault, it
just silently corrupts VRAM/OAM/SRAM or writes to nothing. `HollowShore`
in this repo has a checked-in emulator log (`HollowShore/ConsoleLog`)
that spent its entire recorded session spamming
`[GAME ERROR] GBA Memory: Bad memory Store8: 0x0000070A` — an 8-bit
write into OAM, silently dropped by hardware, never surfaced to whoever
was testing it in real time. This skill exists so that class of bug is
caught automatically, every time, instead of discovered by a human
squinting at a console log later.

## Prerequisites

`libmgba0.10t64`, `libmgba-dev`, and the `mgba` PyPI package. These are
installed by the repo's SessionStart hook; if they're missing, install
with:

```bash
apt-get install -y --no-install-recommends libva2=2.20.0-2build1 libva-drm2=2.20.0-2build1 libva-x11-2=2.20.0-2build1
apt-get install -y --no-install-recommends libmgba0.10t64 libmgba-dev
pip install mgba
```

(The explicit `libva*=2.20.0-2build1` pin works around a broken
`noble-updates` mirror snapshot that 404s on the default candidate
version — pull from plain `noble` instead.)

## Running it

```bash
python3 .claude/skills/gba-verify/verify_rom.py \
  --rom path/to/Game.gba \
  --frames 600 \
  --screenshot-at 1,60,300,600 \
  --out-dir /tmp/verify-out \
  --json /tmp/verify-out/report.json
```

- `--frames`: GBA runs at 60fps, so 600 frames = 10 in-game seconds. Use
  enough frames to get past any boot/logo screen into real gameplay.
- `--screenshot-at`: comma-separated frame numbers to dump a PNG at —
  put one right after boot and one mid-gameplay so you can actually look
  at what rendered, not just trust the log.
- `--press frame:KEY[+KEY]:holdFrames` (repeatable): scripts input, e.g.
  `--press 120:START:2` to press Start for 2 frames starting at frame
  120 (to get from a title screen into gameplay before checking for
  errors). Valid keys: `A B SELECT START RIGHT LEFT UP DOWN R L`.
- `--allow-stub`: don't print/flag STUB-level lines (unimplemented BIOS
  calls or hardware registers mGBA doesn't emulate) as noteworthy. Off
  by default because a STUB often means a code path assumes hardware
  behavior mGBA doesn't provide — worth a look, not automatically fatal.

Exit code is `0` only if no `FATAL`/`ERROR`/`GAME_ERROR` level line was
logged during the run. Treat a non-zero exit the same as a failing test
— do not report the task done until it's clean.

## What it catches vs. what it can't

Catches, because mGBA logs them at GAME_ERROR/ERROR/FATAL when it
happens:
- 8-bit writes to OAM (dropped by hardware — logged as `Cannot Store8 to OAM`)
- out-of-bounds or unmapped memory access
- illegal/undefined CPU opcodes
- DMA misconfiguration mGBA recognizes as invalid

Does **not** catch on its own, because these don't produce a log line —
verify by reading back memory/register state or looking at the
screenshot, not just the exit code:
- 8-bit writes to **VRAM or Palette RAM** — hardware *mirrors* the byte
  into both halves of the 16-bit word instead of erroring, so this is a
  silent data-corruption bug, not a logged one. See
  `.claude/skills/gba-dev/reference/footguns.md`.
- logic bugs, wrong game behavior, softlocks that don't touch invalid
  memory
- anything that only manifests after input sequences longer than what
  you scripted with `--press`

For those, add an assertion in your own test: after `run_frame()` calls,
read the relevant memory address back (`core.memory.u16[addr]` /
`.u32[addr]`) and compare it to what the game logic should have written,
the same way `verify_rom.py` itself is validated (see the two fixture
ROMs described in `.claude/skills/gba-dev/SKILL.md`).
