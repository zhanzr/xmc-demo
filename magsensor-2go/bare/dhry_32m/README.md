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

The built-in self-check passes on both: `Int_Glob=5`, `Bool_Glob=1`,
`Ch_1_Glob=A`, `Ch_2_Glob=B`, `Arr_1_Glob[8]=7`,
`Arr_2_Glob[8][7]=200010` (= `RUN_NUMBER + 10`), both records' `Discr`,
`Enum_Comp`, `Int_Comp`, `Str_Comp`, and `Int_1/2/3_Loc = 5/13/7`.

Note that the score is only meaningful when the self-check passes. A build whose
array accesses run out of bounds reports a *higher* score (the timed loop ends up
doing less work), so always check the "Final values" block.

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

## Deviations from upstream

The ported source is the modified Dhrystone 2.1 variant used by `dhry_100m`, not
the canonical published version. Four changes were needed:

1. **Array dimensions.** This variant's `Proc_8` writes `Arr_1_Glob[8]`, `[9]` and
   `[38]`, and `Arr_2_Glob[8][7]`, `[8][8]`, `[8][9]` and `[28][8]` (`Int_Loc`
   is 8 inside `Proc_8`), so the arrays must be at least `39` and `29x10`. The
   upstream project declared them as `[50]` and `[50][50]`, which costs 10,000
   bytes — 61% of this board's 16 KB SRAM, leaving no room for the stack or stdio
   buffers. They are declared `[40]` and `[30][10]` (1,360 bytes) here, the
   smallest shapes that keep every access in bounds. `Proc_8` only touches a
   handful of elements regardless of size, so the dimensions affect memory
   footprint only, not the instruction count.
   This also makes the self-checks honest: `Arr_1_Glob[8]` really is 7 and
   `Arr_2_Glob[8][7]` really is `10 + run count`, instead of reading past the end
   of the array into whatever happens to follow it.
2. **`Int_3_Loc` is not `register`.** Its address is passed to `Proc_7`, and
   taking the address of a `register` variable is illegal C. GCC only diagnoses it
   when the variable would actually be allocated to a register, so it compiled by
   luck in the original; `register` is only a hint, so dropping it here changes
   nothing semantically.
3. **Static heap arena (`XMC_HEAP_SIZE=4096`).** The board is heap-free by
   default: `_sbrk` fails every allocation, which made `malloc` return `NULL` and
   left the benchmark reading a garbage record pointer while still printing a
   plausible-looking score. newlib's stdio also allocates a `BUFSIZ` buffer per
   stream on first use. The benchmark therefore requests a 4 KB static arena,
   which covers two stdio buffers plus the two work records.
4. **`RUN_NUMBER=200000`.** Down from the upstream 2,000,000, which is unnecessary
   here: 200,000 runs take ~4.9 s, comfortably above Dhrystone's own
   `Too_Small_Time` of 2 s. Edit `RUN_NUMBER` in `src/dhry.h` for longer runs.

`RAM_FUNC`, `PRINTF`, `HAL_GetTick` and the timer macros were already adapted in
the source that was ported; `src/utils.c` maps `HAL_GetTick`/`HAL_Delay` onto the
board's `board_millis()`/`board_delay_ms()`.

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