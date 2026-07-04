# Toolchain setup (verified in this environment)

devkitPro's own package server (`apt.devkitpro.org`) is blocked by this
environment's network policy (returns 403 at the connection level, not
just for one repo — a genuine environment restriction, not something
`add_repo` can work around). The commands below use plain Ubuntu apt
packages instead and were verified end-to-end: compiled a real ARMv4T
Thumb-interworking object, linked a minimal GBA ROM, and booted it in
headless mGBA successfully.

## ARM cross-compiler

```bash
apt-get install -y --no-install-recommends \
  gcc-arm-none-eabi binutils-arm-none-eabi \
  libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```

This is the same GNU toolchain family devkitARM itself is built from
(gcc/binutils/newlib), just packaged by Ubuntu. Confirmed working
invocation for GBA (ARM7TDMI / ARMv4T) code:

```bash
arm-none-eabi-gcc -mthumb -mthumb-interwork -mcpu=arm7tdmi -O2 -c file.c -o file.o
arm-none-eabi-gcc -mthumb-interwork -mcpu=arm7tdmi -nostartfiles -nostdlib -T linker.ld crt0.o file.o -o rom.elf
arm-none-eabi-objcopy -O binary rom.elf rom.gba
```

`libtonc`/devkitARM's own `crt0.s`, linker script, and headers are
**not** available through this toolchain (they live behind the blocked
devkitPro server, and could not be vendored into this session either —
see `butano.md`). If a project needs libtonc specifically, its crt0/
linker script/headers need to be sourced from a session or machine that
can actually reach `devkitpro.org` or `github.com/devkitPro/*`, then
committed into the repo (or added as a submodule) so future sessions
don't hit this same wall.

## mGBA (headless, for `gba-verify`)

```bash
# noble-updates' libva2/-drm2/-x11-2 404 on this mirror snapshot as of
# 2026-07; pin the plain-noble build instead of letting apt pick the
# (missing) updates candidate.
apt-get install -y --no-install-recommends \
  libva2=2.20.0-2build1 libva-drm2=2.20.0-2build1 libva-x11-2=2.20.0-2build1

apt-get install -y --no-install-recommends libmgba0.10t64 libmgba-dev
pip install mgba
```

Verify:

```bash
python3 -c "import mgba.core, mgba.image, mgba.log; print('ok')"
```

If the `pip install mgba` step runs before `libmgba0.10t64` is
installed, the Python package installs but fails to import
(`ImportError: libmgba.so.0.10: cannot open shared object file`) —
install the apt package first, or just rerun `pip install mgba` after.

## Butano

Not installable the same way — it's a source engine (C++), not a
distro package. See `butano.md`.

## This does NOT currently build BrickBreak, SupermarketGBA, or HollowShore as-is

All three existing projects' Makefiles hard-require `DEVKITARM`/
`DEVKITPRO` env vars and `include $(DEVKITARM)/gba_rules`
(`BrickBreak`, `HollowShore`) or reference `$(DEVKITPRO)/devkitARM/bin/`
and `$(DEVKITPRO)/tools/bin/gbafix` directly (`SupermarketGBA`).
`BrickBreak` also links `-lgba` (devkitPro's `libgba`). None of
`gba_rules`, `libgba`, `libtonc`, or `gbafix` are available through
plain apt — they only exist behind the blocked `apt.devkitpro.org`
server. So this apt-based toolchain compiles and links fine for code
that brings its own crt0/linker script (a hand-rolled one, or Butano's
own, once vendored) — confirmed working, see `verify_rom.py`'s test
fixtures referenced in `gba-verify`'s `SKILL.md` — but it will **not**
satisfy any of the three existing project Makefiles without either (a)
getting real devkitPro binaries onto this machine from somewhere that
can reach their server, or (b) rewriting those Makefiles to not depend
on devkitPro-specific rules/libraries. Don't assume `make` works in
those three project directories just because the compiler is installed
— check first.
