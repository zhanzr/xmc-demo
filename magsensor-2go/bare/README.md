# `bare/` — bare-metal projects

Projects in this folder talk to the board layer (`../board/`) directly: no RTOS,
no vendor HAL above `board/`, no framework. Each project is self-contained:

```
<project>/
├── CMakeLists.txt   picks the toolchain, applies the board layer, adds the flash targets
├── build.sh         Pico-style configure + build (CMake + Ninja)
├── src/             application sources
└── README.md        what it does, expected output, build/flash steps
```

| Project                                | What it does                                                        |
| -------------------------------------- | ------------------------------------------------------------------- |
| [`blink_hello`](blink_hello)           | LED blink + die-temperature (DTS) report over the console, with the live CPU/MCLK frequency |
| [`dhry_32m`](dhry_32m)                 | Dhrystone 2.1 benchmark: 41,017 (gcc) / 48,054 (Keil AC6) Dhrystones/second at 32 MHz |
| [`coremark_32m`](coremark_32m)         | CoreMark 1.0.1 benchmark: 42.34 (gcc) / 44.26 (Keil AC6) iterations/second at 32 MHz |
| [`sweep_io`](sweep_io)                 | GPIO discovery sweep: prints the chip ID, then toggles every drive-capable VQFN24 pad one at a time |
| [`st7735_md096_160x80`](st7735_md096_160x80) | ST7735S 0.96" 160x80 LCD bring-up: soft GPIO + USIC0_CH1 hardware 4-wire SPI (mode 3), CCU40 PWM backlight, banner/info/quadrant/gradient/LED patterns |

Build one from its own directory:

```bash
cd blink_hello
bash build.sh          # gcc (default) -> build/
ninja -C build flash   # program through J-Link (SWD)
```

`CMAKE_TOOLCHAIN_FILE` is cached at configure time, so use one build directory
per toolchain (`build/` for gcc, `build-ac6/` for `-DXMC_TOOLCHAIN=armclang`).
See the [board README](../README.md) for hardware and console details.