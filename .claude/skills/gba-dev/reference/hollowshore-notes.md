# HollowShore: what the checked-in ConsoleLog shows

`HollowShore/ConsoleLog` (343KB, checked into the repo) is an emulator
log from a prior play/test session. It is dominated by one repeating
error, with no other error types interspersed:

```
[GAME ERROR] GBA Memory: Bad memory Store8: 0x0000070A
[INFO] GBA DMA: Starting DMA 3 0x03001780 -> 0x07000000 (8400:0100)
```

repeated continuously. This is an 8-bit write into OAM (`0x07000000`
range) — see `footguns.md` #1. Something in `HollowShore/source`
(likely the sprite/OAM update path in `source/graphics/sprites.c` or
wherever per-frame OAM writes happen, given the DMA target is
`0x07000000` and the size/count `8400:0100` looks like an OAM-sized DMA
immediately following) is writing to OAM through a `u8*` somewhere, or
constructing OAM entries byte-by-byte instead of assembling a full
16-bit/32-bit value before the DMA copy.

This was not caught during whatever session produced this log — it just
ran continuously without anyone stopping to read the console. That's
the exact gap `gba-verify` closes: same class of error, but surfaced as
a failing exit code before a task is called done, not discovered later
in a log nobody read at the time.

This file is a note for whoever next works on `HollowShore`, not a fix —
the actual investigation (find the specific byte-write in
`source/graphics/`, confirm with `gba-verify` before and after) is
still open.
