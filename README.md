# XMC1 demo (multi-board)

Bare-metal firmware projects and tooling for multiple **Infineon XMC1000-series**
boards. Each board lives in its own folder with a self-contained `board/`
(clock, LEDs, console, startup, linker script), `cmake/` (toolchain + board
helpers) and `bare/` (projects that talk to the board layer directly). Board-level
docs — hardware, clock tree, build/flash/console — live in each board folder's
README.

## Boards

| Board             | What it is                                                                                                  |
| ----------------- | ----------------------------------------------------------------------------------------------------------- |
| `magsensor-2go/`  | XMC1100-Q024x0064 (Cortex-M0) @ 32 MHz MCLK / 64 MHz PCLK, 64 KB flash / 16 KB SRAM, 2 LEDs (P1.0/P1.1), USIC0 ch0 console (TXD P2.1 / RXD P2.2) on the J-Link-Lite VCP (see its README) |

The `-<chip>` suffix in board folder names keeps it a multi-board/**multi-chip**
repo: e.g. an `evalkit-xmc1400` would sit next to `magsensor-2go`, and any new
XMC1 board can share the repo-root `drivers/` headers.

## Layout

```
<board>/
├── README.md            board docs: hardware, clock tree, build/flash/console
├── board/               board support package (clock, LEDs, console, startup, linker script)
├── board_images/        board photos, embedded in the board README
├── bare/                bare-metal projects (no RTOS / no HAL above the board layer)
│   └── <project>/       one project: src/ + CMakeLists.txt + build.sh + README.md
└── cmake/               toolchain files + board/flash helpers shared by that board's projects
```

## Vendored device headers / CMSIS

The **Infineon XMC1100 device headers + CMSIS 6 core headers** are vendored in
the repo root `drivers/` (trimmed subset of the official `XMC1000_DFP` and `ARM
CMSIS` packs):

```
drivers/
└── CMSIS/
    └── Include/
        ├── Device/Infineon/XMC1100_series/Include/   XMC1100 device headers
        ├── core_cm0.h, core_cm0plus.h, …             CMSIS core headers (M-profile)
        └── m-profile/                                CMSIS compiler-abstraction headers
```

Builds use this by default; `cmake/<board>.cmake` resolves the include paths from
`DRIVERS_ROOT` (which defaults to the repo root `drivers/`). The trimmed tree
covers everything these projects compile; it does **not** include the pack's
XMClib drivers, SVD, Flash algorithms, board support, examples or docs.

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