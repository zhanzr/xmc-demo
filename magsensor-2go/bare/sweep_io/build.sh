#!/usr/bin/env bash
# Build the "magsensor-2go" sweep_io project (GPIO discovery sweep) with
# CMake + Ninja (Pico-style), GNU arm-none-eabi-gcc by default.
# Run with:  bash build.sh    (or ./build.sh on Linux)
#
# Other toolchains (CMAKE_TOOLCHAIN_FILE is cached, so use a separate build dir):
#   bash build.sh -- -DXMC_TOOLCHAIN=armclang   # Keil Arm Compiler 6
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Prefer the MSYS2 mingw64 environment (newer CMake/Ninja) when present.
if [ -d /mingw64/bin ] && ! command -v cmake >/dev/null 2>&1; then
    export PATH="/mingw64/bin:/usr/bin:$PATH"
fi

mkdir -p build
cd build
cmake -G Ninja "$@" ..
ninja