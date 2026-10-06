# Dhrystone 2.1 (`dhry_120m`)

Dhrystone benchmark for the xmc4500-relax-lite board (XMC4500-F100x1024,
Cortex-M4F, 120 MHz). The Dhrystone 2.1 source is the **full upstream version**
(`Arr_1_Dim[50]` / `Arr_2_Dim[50][50]`), not the memory-reduced `dhry_32m` port
used on the 16 KB `magsensor-2go`.

## Results

Measured on hardware (XMC4500-F100x1024, 120 MHz, `RUN_NUMBER = 1000000`, the
built-in self-check passes):

| Toolchain               | Dhrystones/s | DMIPS (÷1757) |
| ----------------------- | -----------: | ------------: |
| GNU `arm-none-eabi-gcc` |      247 831 |         141.0 |
| Keil AC6 `armclang`     |      288 434 |         164.2 |

Check the built-in self-check before trusting a score: all values must match
their expected values, including `Arr_1_Glob[8]=7` and `Arr_2_Glob[8][7]=1000010`
(= `RUN_NUMBER + 10`). A build whose array accesses run out of bounds does less
timed work and therefore reports a *higher* score.

## Build

```bash
cd dhry_120m
bash build.sh                 # gcc      -> build/
ninja -C build flash          # J-Link (SWD)

# Keil AC6: CMAKE_TOOLCHAIN_FILE is cached, so use a separate build directory
cmake -G Ninja -S . -B build-ac6 -DXMC_TOOLCHAIN=armclang
ninja -C build-ac6
```

Without bash:

```bash
cmake -G Ninja -S . -B build
cmake --build build
ninja -C build flash
```

`CMAKE_TOOLCHAIN_FILE` is cached at configure time, so each toolchain needs its
own build directory. Console output goes to USIC1 channel 0 (TXD = P0.5,
RXD = P0.4) at 115200 baud; see the [board README](../../README.md) for the wiring.

Useful options: `-DXMC_HEAP_SIZE=<bytes>` (default 4096) sizes the static
`_sbrk` arena, `-DXMC_LTO=ON` enables GCC LTO, and `-DBENCH_OPT="..."` overrides
the optimization flags.

## Deviations from `magsensor-2go`'s `dhry_32m`

1. **`RUN_NUMBER=1000000`.** At 120 MHz a run is ~4 s, above Dhrystone's 2 s
   minimum (200000 gave only ~1.3 s and the benchmark refused a score). The
   source is otherwise byte-for-byte the Dhrystone 2.1 code.
2. **`XMC_HEAP_SIZE=4096`** (`CMakeLists.txt`). Without it `malloc` returns NULL
   and the benchmark reads a garbage record pointer while still printing a
   plausible-looking score.
3. **Full upstream arrays** (`Arr_1_Dim[50]`, `Arr_2_Dim[50][50]`): the XMC4500
   has 128 KB+ of SRAM, so the reduced `[40]` / `[30][10]` shapes used on the
   16 KB `magsensor-2go` are not needed.

`dhry_2.c` is kept even though it is not compiled: it is the "no main" variant of
the benchmark and documents the intended behaviour of the procedures.
