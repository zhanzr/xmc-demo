# XMC demo (multi-board)

Bare-metal firmware projects and tooling for multiple **Infineon XMC** boards.
Each board lives in its own folder with a self-contained `board/` (clock, LEDs,
console, startup, linker script), `cmake/` (toolchain + board helpers) and
`bare/` (projects that talk to the board layer directly). Board-level docs —
hardware, clock tree, build/flash/console — live in each board folder's README.

## Boards

| Board                  | Series | What it is                                                                                                  |
| ---------------------- | ------ | ----------------------------------------------------------------------------------------------------------- |
| `magsensor-2go/`       | XMC1   | XMC1100-Q024x0064 (Cortex-M0) @ 32 MHz MCLK / 64 MHz PCLK, 64 KB flash / 16 KB SRAM, 2 LEDs (P1.0/P1.1), USIC0 ch0 console (TXD P2.1 / RXD P2.2) on the J-Link-Lite VCP (see its README) |
| `xmc4500-relax-lite/`  | XMC4   | XMC4500-F100x1024 (Cortex-M4F) @ 120 MHz fCPU/fPB, 1024 KB flash / 64+64+32 KB SRAM, 2 LEDs (P1.0/P1.1), USIC1 ch0 console (TXD P0.5 / RXD P0.4) on the external COM bridge (see its README) |

Boards of other XMC series (XMC7000/Cortex-M7, …) are expected to sit next to
them in their own folder and reuse the repo-root `drivers/` headers.

## Vendored device headers / CMSIS

The **Infineon device headers + CMSIS core headers** are vendored in the repo
root `drivers/`, one folder per XMC series (`drivers/<series>/`, a trimmed subset
of the official device-family pack and the `ARM CMSIS` pack):

```
drivers/
├── xmc1/
│   └── CMSIS/
│       └── Include/
│           ├── Device/Infineon/XMC1100_series/Include/   XMC1100 device headers
│           ├── core_cm0.h, core_cm0plus.h, …             CMSIS core headers (M-profile)
│           └── m-profile/                                CMSIS compiler-abstraction headers
└── xmc4/
    ├── CMSIS/
    │   └── Include/
    │       ├── Device/Infineon/XMC4500_series/Include/   XMC4500 device headers
    │       ├── core_cm4.h, …                             CMSIS core headers (M-profile)
    │       └── m-profile/                                CMSIS compiler-abstraction headers
    └── XMClib/
        ├── inc/                                          XMClib headers (xmc_scu, xmc_gpio, …)
        └── src/                                          XMClib sources (compiled on demand)
```

Builds use this by default; `cmake/<board>.cmake` resolves the include paths from
`DRIVERS_ROOT` (which defaults to the repo root `drivers/`) plus the series
folder name. The `xmc1` tree is device-headers/CMSIS only; `xmc4` additionally
vendors XMClib (the `xmc4500-relax-lite` board compiles just `xmc4_scu.c`, for
the die temperature and EVR13/EVR33 monitors). Neither tree includes the packs'
SVD, Flash algorithms, board support, examples or docs.

To build against the **full** official packs instead (e.g. to pull in XMClib or a
device header that is not vendored), point the roots at them when configuring:

```bash
cmake -G Ninja -S . -B build \
  -DXMC_DFP_ROOT="D:/Arm/Packs/Infineon/XMC1000_DFP/2.12.0" \
  -DCMSIS_ROOT="D:/Arm/Packs/ARM/CMSIS/6.3.0" ..
```

## Toolchain / environment

* GNU `arm-none-eabi-gcc` (default) or Keil AC6 `armclang`
  (`-DXMC_TOOLCHAIN=armclang`, C + GNU as/ld).
* CMake + Ninja (Pico-style; the MSYS2 mingw64 `build.sh` adds them to `PATH`).
* SEGGER J-Link (SWD flashing) — `ninja flash`.

Default GCC root is `D:/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi`
(`bin/arm-none-eabi-gcc`); override the toolchain paths with
`-DARM_GCC_ROOT=…` / `-DARMCLANG_ROOT=…`.

## Build configuration

Configure and build each project from its own directory
(`<board>/bare/<project>`). No absolute paths are needed — the build directory is
a relative subfolder of the project.

```bash
cd magsensor-2go/bare/blink_hello

# default: gcc + the project's hard-coded optimization level
bash build.sh          # == mkdir -p build && cd build && cmake -G Ninja .. && ninja
ninja -C build flash   # program via J-Link (SWD)

# other toolchain: use one build dir per toolchain
# (CMAKE_TOOLCHAIN_FILE is cached at configure time)
cmake -G Ninja -S . -B build-ac6 -DXMC_TOOLCHAIN=armclang
ninja -C build-ac6
```

**Optimization levels** are passed to the board-apply CMake function in each
project's `CMakeLists.txt` (bare-metal demos use `-O1`).