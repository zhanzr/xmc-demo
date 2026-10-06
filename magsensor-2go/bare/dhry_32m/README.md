# Dhrystone 2.1 (`dhry_32m`)

Dhrystone benchmark for the magsensor-2go board (XMC1100-Q024x0064, Cortex-M0+,
32 MHz). Ported from `dhry_100m` in the `f4-demo` project; see the "Deviations
from upstream" section, because the source had to be adapted to fit this part's
16 KB SRAM.

## Measured results

Verified on hardware at 32 MHz, `RUN_NUMBER = 200000`, console at 115200 baud.
Each iteration is ~4.9 s of benchmark time.

| Toolchain                             | Compiler flags                                  | Dhrystones/second |
| ------------------------------------- | ----------------------------------------------- | ----------------- |
| GNU arm-none-eabi-gcc 15.3.1          | `-Ofast -ffp-contract=fast -funroll-loops`      | **41,017**        |
| Keil Arm Compiler 6 (armclang 24)     | `-Ofast -ffp-contract=fast -funroll-loops`      | **48,054**        |

The built-in self-check passes on both, all values matching their expected
values, including `Arr_1_Glob[8]=7` and `Arr_2_Glob[8][7]=200010`
(= `RUN_NUMBER + 10`).

Check it before trusting a score: a build whose array accesses run out of bounds
does less timed work and therefore reports a *higher* score.

## Build

```bash
cd dhry_32m
bash build.sh                                              # gcc      -> build/
BUILD_DIR=build-ac6 bash build.sh -DXMC_TOOLCHAIN=armclang # Keil AC6 -> build-ac6/

ninja -C build flash      # or: ninja -C build-ac6 flash  (J-Link, SWD)
```

Without bash:

```bash
cmake -G Ninja -S . -B build
cmake --build build
ninja -C build flash
```

`CMAKE_TOOLCHAIN_FILE` is cached at configure time, so each toolchain needs its
own build directory. Console output goes to USIC0 channel 0 at 115200 baud; see
the [board README](../../README.md) for the wiring.

Useful options: `-DXMC_HEAP_SIZE=<bytes>` (default 4096) sizes the static
`_sbrk` arena, `-DXMC_LTO=ON` enables GCC LTO, and `-DBENCH_OPT="..."` overrides
the optimization flags.

### ArmClang and `-Omax`

`-Omax` is ArmClang-only, so it goes through `BENCH_OPT_C` (C sources only — GCC
rejects `-Omax`, and so do GNU `as`/`ld`):

```bash
cmake -G Ninja -S . -B build-ac6 -DXMC_TOOLCHAIN=armclang -DBENCH_OPT_C="-Omax -fno-lto"
```

`-fno-lto` is required: `-Omax` makes ArmClang emit LLVM bitcode objects that GNU
`ld` cannot read, so the link fails with `undefined reference to 'main'`. With both
flags it builds and measures the same 48,054, so the defaults are kept.

## Deviations from upstream

1. **Array dimensions.** This variant's `Proc_8` writes `Arr_1_Glob[38]` and
   `Arr_2_Glob[28][8]`, so upstream's `[50]`/`[50][50]` (10 KB, 61% of this
   board's 16 KB SRAM) is reduced to `[40]` and `[30][10]`, the smallest shapes
   keeping every access in bounds. `Proc_8` touches a fixed number of elements
   regardless of size, so the score is unaffected. Side benefit: `Arr_1_Glob[8]`
   and `Arr_2_Glob[8][7]` now read real values instead of adjacent memory.
2. **`Int_3_Loc` is not `register`.** Its address is passed to `Proc_7`, and
   taking the address of a `register` variable is invalid C.
3. **`XMC_HEAP_SIZE=4096`** (`CMakeLists.txt`). Without it `malloc` returns NULL
   and the benchmark reads a garbage record pointer while still printing a
   plausible-looking score.
4. **`RUN_NUMBER=200000`**, down from 2,000,000: ~4.9 s per run, above
   Dhrystone's own 2 s minimum.

## Layout

```
dhry_32m/
├── CMakeLists.txt    toolchain selection, heap size, board layer, flash targets
├── build.sh          Pico-style configure + build
└── src/
    ├── main.c        board init, runs dhry_main() in a loop, reports the clock
    ├── utils.c/.h    Dhrystone hooks: clock, console output
    ├── dhry.h        RUN_NUMBER and array dimensions (see above)
    ├── dhry_1.c      main benchmark body
    └── dhry_2.c      Proc_1..8 / Func_1..3
```

`dhry_2.c` is kept even though it is not compiled: it is the "no main" variant of
the benchmark and documents the intended behaviour of the procedures.