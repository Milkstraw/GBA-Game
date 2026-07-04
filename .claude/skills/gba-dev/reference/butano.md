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

## It is not vendored in this repo, and could not be added this session

Getting Butano's actual source into a project normally means:

```bash
git submodule add https://github.com/GValiente/butano.git external/butano
```

That command needs a plain `git clone`/`submodule add` against
`github.com`, which this session's GitHub access does not permit — this
session (like any Claude Code web session) is scoped to specific
repositories, and once repos exist in a session, only the *same owner*
can be added afterward (attempting to add `GValiente/butano` alongside
this repo's `milkstraw/*` scope failed with `cross-tier adds are not
supported in v1`). Note this is a per-session GitHub scoping limitation,
separate from the environment's network policy issue described in
`toolchain-setup.md`.

`raw.githubusercontent.com` (unscoped) was reachable and can fetch
*individual known files* from public repos, but not an entire repo tree
(no directory listing without the GitHub API, which is scoped the same
way `github.com` is). That's enough to read one file at a time if you
already know its exact path, not enough to reliably vendor an engine.

**What this means practically:** the first time a new game actually
needs Butano vendored, do it from wherever has real `github.com` access
— your local machine, or a fresh Claude Code session started with
`GValiente/butano` as its (only, or first) repo source — then commit
the result (or add it as a proper git submodule) so every session after
that has it without hitting this wall again.

## Requirements once vendored

- devkitARM or the Wonderful Toolchain for the actual cross-compiler
  (see `toolchain-setup.md` for why devkitARM itself isn't reachable
  here, and the apt-based substitute that was verified working for
  plain ARMv4T/Thumb code — confirm separately whether Butano's build
  system accepts a non-devkitPro `arm-none-eabi-*` toolchain pointed at
  via `DEVKITARM`, since Butano's Makefiles were written assuming
  devkitPro's layout).
- Python (2 or 3) for the asset-conversion tools bundled with the
  engine.
- Getting-started docs: `gvaliente.github.io/butano/getting_started.html`
  (not reachable from this session either — same host-blocking as
  general web browsing here; fetch from a machine/session with normal
  internet access).

## Reference

- Repo: `github.com/GValiente/butano`
- `awesome-gbadev` (github.com/gbadev-org/awesome-gbadev) lists Butano
  alongside other engines/libraries/tools if the stack decision ever
  needs revisiting for a specific game.
