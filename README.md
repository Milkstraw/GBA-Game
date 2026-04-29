# Brick Breaker GBA

A Breakout clone for the Game Boy Advance, written in C using direct hardware
register access and GBA Video Mode 3 (16-bit bitmap, 240×160).

## Features

- 8×6 brick grid (48 bricks per level) with 6 rainbow color rows
- Ball physics with fixed-point arithmetic (no floating point)
- Speed increases with each brick broken
- 5 levels with increasing base speed
- 3 lives per game, score display
- Title, pause, game-over, and win screens
- Dirty-rectangle rendering (no full-screen clear per frame)

## Controls

| Button        | Action                        |
|---------------|-------------------------------|
| D-Pad Left/Right | Move paddle               |
| A             | Launch ball                   |
| START         | Pause / Resume                |
| SELECT        | Return to title (Game Over screen) |

## Building

### Prerequisites

Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) with the GBA
development package, then set environment variables:

```bash
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=$DEVKITPRO/devkitARM
export LIBGBA=$DEVKITPRO/libgba
export PATH=$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH
```

On Ubuntu/Debian you can install devkitPro via their pacman repository:

```bash
wget https://apt.devkitpro.org/install-devkitpro-pacman
chmod +x install-devkitpro-pacman
sudo ./install-devkitpro-pacman
sudo dkp-pacman -S gba-dev
```

### Compile

```bash
make
```

This produces `BrickBreakerGBA.gba`.

### Play

Open `BrickBreakerGBA.gba` in a GBA emulator:

- [mGBA](https://mgba.io/) (recommended — most accurate)
- VisualBoyAdvance-M
- Or flash to a GBA cartridge with a flash cart.

## Technical Notes

- GBA Video Mode 3: 16-bit direct color bitmap, 240×160 resolution
- Fixed-point arithmetic with 8 fractional bits (scale factor 256)
- Colors use BGR555 format: `RGB15(r,g,b)` macro packs 5 bits each
- Per-frame render: only erase/redraw ball and paddle (48 bricks are static)
- Font: custom 5×7 bitmap, column-major, LSB = top row
- No BIOS calls; only raw hardware register access + devkitPro startup/linker
