# CoreMark 1.0.1 (`coremark_120m`)

CoreMark benchmark for the xmc4500-relax-lite board (XMC4500-F100x1024,
Cortex-M4F, 120 MHz). Ported from `coremark_32m` in the `magsensor-2go` project;
the EEMBC CoreMark 1.0.1 sources are kept unmodified.

## Results

Measured on hardware (XMC4500-F100x1024, 120 MHz, `ITERATIONS = 4000`, each run
≥10 s and ends with `Correct operation validated`):

| Toolchain               | CoreMark (iterations/s) | Optimisation flags                             |
| ----------------------- | ----------------------: | ---------------------------------------------- |
| GNU `arm-none-eabi-gcc` |              322.945261 | `-Ofast -ffp-contract=fast -funroll-all-loops` |
| Keil AC6 `armclang`     |              326.503959 | `-Ofast -ffp-contract=fast -funroll-loops`     |

## Build

```bash
cd coremark_120m
bash build.sh                 # gcc      -> build/
ninja -C build flash          # J-Link (SWD)

# Keil AC6: CMAKE_TOOLCHAIN_FILE is cached, so use a separate build directory
cmake -G Ninja -S . -B build-ac6 -DXMC_TOOLCHAIN=armclang
ninja -C build-ac6
```

Console output goes to USIC1 channel 0 (TXD = P0.5, RXD = P0.4) at 115200 baud; see
the [board README](../../README.md) for the wiring.

Useful options: `-DXMC_HEAP_SIZE=<bytes>` (default 4096) sizes the static
`_sbrk` arena, `-DXMC_LTO=ON` enables GCC LTO, and `-DBENCH_OPT="..."` overrides
the optimization flags.

## Configuration notes

- `MEM_METHOD = MEM_STATIC`, `ITERATIONS = 4000`.
- `XMC_HEAP_SIZE = 4096` in `CMakeLists.txt`: CoreMark never calls `malloc`, but
  newlib's stdio allocates a buffer per stream, so the board's default
  fail-everything `_sbrk` breaks the first `printf`.
- `-funroll-all-loops` is GCC-only; AC6 gets `-funroll-loops`.

The reference implementation is kept intact, including `LICENSE.md`. Official
run and reporting rules are documented by EEMBC in the CoreMark 1.0 release; the
result line this port prints follows them.
