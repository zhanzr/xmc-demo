# CoreMark 1.0.1 (`coremark_32m`)

CoreMark benchmark for the magsensor-2go board (XMC1100-Q024x0064, Cortex-M0+,
32 MHz). Ported from `coremark_100m` in the `f4-demo` project.

## Measured results

Verified on hardware at 32 MHz, `ITERATIONS = 500`, console at 115200 baud. Each
run takes ~11.3-11.8 s, above the 10 s that CoreMark requires for a valid result.

| Toolchain                             | Compiler flags                                       | Iterations/second |
| ------------------------------------- | ---------------------------------------------------- | ----------------- |
| GNU arm-none-eabi-gcc 15.3.1          | `-Ofast -ffp-contract=fast -funroll-all-loops`       | **42.34**         |
| Keil Arm Compiler 6 (armclang 24)     | `-Ofast -ffp-contract=fast -funroll-loops`           | **44.26**         |

`CoreMark Size: 666`, and the run ends with `Correct operation validated`.

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

- `MEM_METHOD = MEM_STATIC`, `ITERATIONS = 500` (CoreMark rejects runs under 10 s).
- `XMC_HEAP_SIZE = 4096` in `CMakeLists.txt`: CoreMark never calls `malloc`, but
  newlib's stdio allocates a buffer per stream, so the board's default
  fail-everything `_sbrk` breaks the first `printf`.
- `-funroll-all-loops` is GCC-only; AC6 gets `-funroll-loops`. `-Omax` is not
  usable here, unlike on the nano-f411 board: with the required `-fno-lto` it
  links, but the extra inlining overflows this part's 64 KB flash by ~3 KB.

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