#!/usr/bin/env python3
"""Headless mGBA verification harness for GBA ROMs.

Loads a .gba file into libmgba (no display needed), runs it for a fixed
number of frames, captures mGBA's own hardware-level diagnostics (invalid
VRAM/OAM/Palette writes, unimplemented BIOS calls, etc.), and saves
screenshots. Exits non-zero if anything at ERROR/FATAL/GAME_ERROR level
was logged, so it can gate a task as "done" the same way a test suite would.

Usage:
  python3 verify_rom.py --rom game.gba --frames 600 \
      --screenshot-at 1,60,300,600 --out-dir /tmp/verify

  # with scripted input (press A at frame 120, held for 4 frames)
  python3 verify_rom.py --rom game.gba --frames 300 --press 120:A:4
"""
import argparse
import json
import os
import sys

from mgba._pylib import ffi
import mgba.core
import mgba.image
import mgba.log

KEY_BITS = {
    "A": 0x0001, "B": 0x0002, "SELECT": 0x0004, "START": 0x0008,
    "RIGHT": 0x0010, "LEFT": 0x0020, "UP": 0x0040, "DOWN": 0x0080,
    "R": 0x0100, "L": 0x0200,
}

# Anything at this mgba.log level or more severe fails the run.
FAILING_LEVELS = {
    mgba.log.Logger.FATAL: "FATAL",
    mgba.log.Logger.ERROR: "ERROR",
    mgba.log.Logger.GAME_ERROR: "GAME_ERROR",
}


class Capture(mgba.log.Logger):
    def __init__(self):
        super().__init__()
        self.lines = []

    def log(self, category, level, message):
        text = ffi.string(message).decode("utf-8", "replace")
        self.lines.append((self.category_name(category), level, text))


def parse_press(spec):
    """Parse "frame:KEY[+KEY...]:holdFrames" into (frame, mask, hold)."""
    frame_s, keys_s, hold_s = spec.split(":")
    mask = 0
    for key in keys_s.split("+"):
        key = key.strip().upper()
        if key not in KEY_BITS:
            raise ValueError(f"unknown key '{key}', expected one of {sorted(KEY_BITS)}")
        mask |= KEY_BITS[key]
    return int(frame_s), mask, int(hold_s)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rom", required=True, help="path to the .gba file to run")
    ap.add_argument("--frames", type=int, default=300, help="number of frames to run (60/sec)")
    ap.add_argument("--screenshot-at", default="", help="comma-separated frame numbers to save a PNG at")
    ap.add_argument("--press", action="append", default=[], help='frame:KEY[+KEY]:holdFrames, e.g. 60:START:2 (repeatable)')
    ap.add_argument("--out-dir", default=".", help="directory to write screenshots/report into")
    ap.add_argument("--json", default=None, help="path to write a machine-readable JSON report")
    ap.add_argument("--allow-stub", action="store_true", help="don't treat STUB (unimplemented BIOS/register) log lines as noteworthy")
    args = ap.parse_args()

    if not os.path.isfile(args.rom):
        print(f"FAIL: ROM not found: {args.rom}")
        return 2

    os.makedirs(args.out_dir, exist_ok=True)
    screenshot_frames = {int(x) for x in args.screenshot_at.split(",") if x.strip()}
    presses = [parse_press(p) for p in args.press]

    capture = Capture()
    mgba.log.Logger.install_default(capture)

    core = mgba.core.load_path(args.rom)
    if core is None:
        print(f"FAIL: mgba could not load '{args.rom}' as a GBA ROM (bad header/format?)")
        return 2

    width, height = core.desired_video_dimensions()
    image = mgba.image.Image(width, height)
    core.set_video_buffer(image)
    core.reset()

    active_presses = []  # (mask, remaining_frames)
    for frame in range(1, args.frames + 1):
        for f0, mask, hold in presses:
            if f0 == frame:
                active_presses.append([mask, hold])

        keys = 0
        for entry in active_presses:
            keys |= entry[0]
        core.set_keys(keys)

        core.run_frame()

        for entry in active_presses:
            entry[1] -= 1
        active_presses = [e for e in active_presses if e[1] > 0]

        if frame in screenshot_frames:
            path = os.path.join(args.out_dir, f"frame{frame:05d}.png")
            with open(path, "wb") as fh:
                image.save_png(fh)

    failures = [l for l in capture.lines if l[1] in FAILING_LEVELS]
    stubs = [l for l in capture.lines if l[1] == mgba.log.Logger.STUB]
    if args.allow_stub:
        stubs = []

    print(f"ran {args.frames} frames of {args.rom}")
    print(f"log lines captured: {len(capture.lines)}")
    if failures:
        print(f"FAIL: {len(failures)} error-level log line(s):")
        for cat, level, msg in failures:
            print(f"  [{FAILING_LEVELS[level]}] {cat}: {msg}")
    else:
        print("no FATAL/ERROR/GAME_ERROR log lines")
    if stubs:
        print(f"note: {len(stubs)} STUB (unimplemented) log line(s), not treated as failures:")
        for cat, _level, msg in stubs[:10]:
            print(f"  [STUB] {cat}: {msg}")

    if args.json:
        report = {
            "rom": args.rom,
            "frames_run": args.frames,
            "passed": not failures,
            "failures": [{"category": c, "level": FAILING_LEVELS[l], "message": m} for c, l, m in failures],
            "stub_count": len(stubs),
            "total_log_lines": len(capture.lines),
        }
        with open(args.json, "w") as fh:
            json.dump(report, fh, indent=2)

    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
