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

| Project                        | What it does                                                                 |
| ------------------------------ | ---------------------------------------------------------------------------- |
| [`blink_hello`](blink_hello)   | LED blink + die-temperature / EVR13 / EVR33 report over the console, with the live CPU/fPB frequency |
| [`dhry_120m`](dhry_120m)       | Dhrystone 2.1 benchmark at 120 MHz (score not measured on hardware yet)      |
| [`coremark_120m`](coremark_120m) | CoreMark 1.0.1 benchmark at 120 MHz (score not measured on hardware yet)   |

Build one from its own directory:

```bash
cd blink_hello
bash build.sh          # gcc (default) -> build/
ninja -C build flash   # program through J-Link (SWD)
```

`CMAKE_TOOLCHAIN_FILE` is cached at configure time, so use one build directory
per toolchain (`build/` for gcc, `build-ac6/` for `-DXMC_TOOLCHAIN=armclang`).
See the [board README](../README.md) for hardware and console details.
