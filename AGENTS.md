# AGENTS.md — working agreements for this repo

## Toolchains

**Build with the default GCC toolchain only** (GNU `arm-none-eabi-gcc`):

```bash
cd <board>/bare/<project>
bash build.sh              # == mkdir -p build && cd build && cmake -G Ninja .. && ninja
```

Do **not** configure or build `build-ac6` (Keil AC6 / `armclang`) unless the task
explicitly asks for it. Performance is not a goal for board bring-up work, so a
second toolchain only adds noise.

When a task *does* require AC6 (e.g. comparing code size or optimisation between
compilers), use the separate build directory, because `CMAKE_TOOLCHAIN_FILE` is
cached at configure time:

```bash
cmake -G Ninja -S . -B build-ac6 -DXMC_TOOLCHAIN=armclang
ninja -C build-ac6
```

## Board facts to trust

- `magsensor-2go` is an **XMC1100-Q024x0064** in a **VQFN24** package — verified on
  hardware: `SCU_GENERAL->IDCHIP` and flash `0x10000F00` both read `0x00011062`,
  which is the `-Q024` package code.
- `LED1 = P1.0`, `LED2 = P1.1` — confirmed working on the board.
- Console = USIC0 channel 0, TXD `P2.1`, RXD `P2.2` on the J-Link-Lite VCP
  (115200 8N1).
- SWD = `P1.2` (SWCLK) / `P1.3` (SWDIO), per the board manual. Keep both out of
  any pin sweep.
- `PORT0`/`PORT1` `PDISC` is **read-only** — writing it raises a HardFault (the DFP
  header declares it `__I`). Only `PORT2` has analog-capable pads, so only clear
  `PORT2->PDISC` (it resets to `0xFFFF`).
- P2.7 and P2.8 are the same physical pad (5) on VQFN24, and P2.2–P2.9 are
  `STD_IN/AN` pads per datasheet Table 6, i.e. no output driver.

## Code style

Keep comments terse and factual — no long explanatory blocks or restated history.
Match the surrounding file's conventions.