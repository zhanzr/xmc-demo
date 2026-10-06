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

Build one from its own directory:

```bash
cd blink_hello
bash build.sh          # gcc (default) -> build/
ninja -C build flash   # program through J-Link (SWD)
```

`CMAKE_TOOLCHAIN_FILE` is cached at configure time, so use one build directory
per toolchain (`build/` for gcc, `build-ac6/` for `-DXMC_TOOLCHAIN=armclang`).
See the [board README](../README.md) for hardware and console details.