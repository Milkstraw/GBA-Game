# External references

- [Tonc](https://gbadev.net/tonc/intro.html) — the canonical GBA C
  tutorial: memory map, video modes, sprites, DMA, interrupts,
  fixed-point math, with runnable example code per chapter.
- [libtonc](https://github.com/devkitPro/libtonc) — the library that
  accompanies Tonc; used by `SupermarketGBA` in this repo.
- [awesome-gbadev](https://github.com/gbadev-org/awesome-gbadev) —
  curated list of GBA dev tools, libraries, docs, and community
  resources. Good first stop if a footgun or pattern isn't covered here.
- [Butano](https://github.com/GValiente/butano) — the chosen engine for
  new games in this repo; see `butano.md` for the local caveats.
- [GBATEK](http://problemkaputt.de/gbatek.htm) — the exhaustive
  low-level hardware reference (register-by-register, bit-by-bit).
  Ground truth for anything `footguns.md` doesn't cover, but dense —
  use it to answer a specific question, not as a first read.
- [mGBA](https://mgba.io/) — the emulator used by `gba-verify`; also
  the recommended emulator for manual play-testing (most
  hardware-accurate of the common options).

None of these were fetchable directly from this session (general web
browsing is blocked by this environment's network policy; GitHub access
is scoped per-session — see `toolchain-setup.md` and `butano.md`). Pull
specific pages via a session/machine with normal internet access when
detail beyond `footguns.md` is needed.
