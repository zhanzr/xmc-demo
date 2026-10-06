#!/usr/bin/env bash
# Build the Dhrystone 2.1 benchmark for the "magsensor-2go" board (XMC1100-Q024x0064)
# with CMake + Ninja (Pico-style), GNU arm-none-eabi-gcc by default.
# Run with:  bash build.sh    (or ./build.sh on Linux)
#
# CMAKE_TOOLCHAIN_FILE is cached at configure time, so give each toolchain its own
# build directory:
#   bash build.sh                                             # gcc      -> build/
#   BUILD_DIR=build-ac6 bash build.sh -DXMC_TOOLCHAIN=armclang # Keil AC6 -> build-ac6/
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Prefer the MSYS2 mingw64 environment (newer CMake/Ninja) when present.
if [ -d /mingw64/bin ] && ! command -v cmake >/dev/null 2>&1; then
    export PATH="/mingw64/bin:/usr/bin:$PATH"
fi

BUILD_DIR="${BUILD_DIR:-build}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake -G Ninja "$@" ..
ninja