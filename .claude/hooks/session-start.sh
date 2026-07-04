#!/bin/bash
set -euo pipefail

if [ "${CLAUDE_CODE_REMOTE:-}" != "true" ]; then
  exit 0
fi

need_toolchain=1
if command -v arm-none-eabi-gcc >/dev/null 2>&1; then
  need_toolchain=0
fi

need_mgba=1
if python3 -c "import mgba.core, mgba.image, mgba.log" >/dev/null 2>&1; then
  need_mgba=0
fi

if [ "$need_toolchain" -eq 0 ] && [ "$need_mgba" -eq 0 ]; then
  echo "gba-dev toolchain and mgba already present, skipping install"
  exit 0
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq

if [ "$need_toolchain" -eq 1 ]; then
  # ARM7TDMI/ARMv4T Thumb-interworking cross-compiler for GBA. devkitPro's
  # own server is blocked by this environment's network policy; this is the
  # plain-Ubuntu equivalent, confirmed to produce a bootable GBA ROM.
  apt-get install -y --no-install-recommends \
    gcc-arm-none-eabi binutils-arm-none-eabi \
    libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
fi

if [ "$need_mgba" -eq 1 ]; then
  # noble-updates' libva2/-drm2/-x11-2 404 on this mirror snapshot; pin the
  # plain-noble build instead of the (missing) updates candidate.
  apt-get install -y --no-install-recommends \
    libva2=2.20.0-2build1 libva-drm2=2.20.0-2build1 libva-x11-2=2.20.0-2build1 \
    || apt-get install -y --no-install-recommends libva2 libva-drm2 libva-x11-2

  apt-get install -y --no-install-recommends libmgba0.10t64 libmgba-dev
  pip install --quiet mgba
fi

echo "gba-dev toolchain setup complete"
