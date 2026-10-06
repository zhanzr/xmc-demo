# CoreMark 1.0.1 (`coremark_32m`)

CoreMark benchmark for the magsensor-2go board (XMC1100-Q024x0064, Cortex-M0+,
32 MHz). Ported from `coremark_100m` in the `f4-demo` project.

## Measured results

Verified on hardware at 32 MHz, `ITERATIONS = 500`, console at 115200 baud. Each
run takes ~11.3-11.8 s, above the 10 s that CoreMark requires for a valid
result, and ends with `Correct operation validated`.

| Toolchain                             | Compiler flags                                       | Iterations/second |
| ------------------------------------- | ---------------------------------------------------- | ----------------- |
| GNU arm-none-eabi-gcc 15.3.1          | `-Ofast -ffp-contract=fast -funroll-all-loops`       | **42.34**         |
| Keil Arm Compiler 6 (armclang 24)     | `-Ofast -ffp-contract=fast -funroll-loops`           | **44.26**         |

`CoreMark Size: 666`. Correctness CRCs, identical on both toolchains:

```
seedcrc      : 0xe9f5    crclist   : 0xe714
crcmatrix    : 0x1fd7    crcstate  : 0x8e3a
```

(`crcfinal` depends on the iteration count and is reported separately.)

## Build

```bash
cd coremark_32m
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

## Configuration notes

- `MEM_METHOD = MEM_STATIC` (`src/core_portme.h`). The benchmark itself never
  calls `malloc`; the state array is a static buffer sized for this part.
- `ITERATIONS = 500`. CoreMark rejects runs shorter than 10 s. Measured throughput
  is ~42 iterations/s, so 500 iterations gives ~11.8 s. Override on the compiler
  command line (`-DITERATIONS=...`) for a longer run.
- `XMC_HEAP_SIZE = 4096` (set in `CMakeLists.txt`). CoreMark's own allocations are
  compiled out, but newlib's stdio allocates a `BUFSIZ` buffer per stream on first
  use; with the board's default fail-everything `_sbrk`, the first `printf` prints
  a `Balloc succeeded` assertion and the run stops mid-report.
- `-funroll-all-loops` is GCC-only. Arm Compiler 6 rejects it with
  `-Wignored-optimization-argument`, so `BENCH_OPT` defaults to `-funroll-loops`
  under `XMC_TOOLCHAIN=armclang`, and `FLAGS_STR` in `src/core_portme.h` reports
  whichever was actually used.
- Two format specifiers in `coremark_1_0_1/core_main.c` were changed from `%lu`
  to `%u` (`CoreMark Size`, `Total ticks`, `Iterations`). The arguments are
  `unsigned int`, which AC6 flags and GCC does not. No behavioural change.

## Layout

```
coremark_32m/
├── CMakeLists.txt        toolchain selection, heap size, board layer, flash targets
├── build.sh              Pico-style configure + build
├── src/
│   ├── main.c            board init, seeds the run, prints the summary line
│   ├── core_portme.c/.h  timing hooks, port configuration (ITERATIONS, MEM_STATIC)
│   └── utils.c/.h        CoreMark hooks: clock, console output
└── coremark_1_0_1/       unmodified EEMBC CoreMark 1.0.1 sources
                          (except the three %lu -> %u format fixes above)
```

The reference implementation is kept intact, including `LICENSE.md`. Official
run and reporting rules are documented by EEMBC in the CoreMark 1.0 release; the
result line this port prints follows them.