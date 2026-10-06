# blink_hello (xmc4500-relax-lite, XMC4500)

Smoke test / bring-up project for the **xmc4500-relax-lite** board: brings up the
board layer, prints the live CPU and peripheral clocks over the console, blinks
both LEDs and reports the die temperature and EVR13/EVR33 supplies once a second.

## What it does

* `board_init()` → clocks (already set by `SystemInit()`), LEDs, SysTick and the
  USIC1 channel 0 console.
* prints the banner (fCPU/fPB, console pins, LED pins),
* toggles LED1 (`P1.0`) and LED2 (`P1.1`) every **500 ms** (active high),
* every **1000 ms** starts a DTS conversion, reads the result and the EVR13/EVR33
  supply monitors from the XMClib SCU driver.

The die temperature uses the DAVE reference formula for this board,
`T[degC] = (DTSTAT.RESULT - 605) / 2.05`, evaluated as integer millidegrees.

## Expected console output

External COM bridge (**COM93**) at **115200 8N1** (TXD `P0.5`, RXD `P0.4`):

```text
xmc4500-relax-lite / XMC4500-F100x1024 / blink_hello
  CPU clock (fCPU)   : 120000000 Hz (120.000 MHz)
  Peripheral (fPB)   : 120000000 Hz (120.000 MHz)
  Console            : USIC1_CH0 TXD=P0.5 RXD=P0.4, 115200 8N1
  LEDs               : LED1=P1.0 LED2=P1.1 (active high)

[      34 ms] DTS: 35.000 C  EVR13: 1330 mV  EVR33: 3300 mV
[    1034 ms] DTS: 35.000 C  EVR13: 1330 mV  EVR33: 3300 mV
```

## Build / flash

```bash
bash build.sh                 # gcc (arm-none-eabi), output in build/
ninja -C build flash          # J-Link (SWD): program .hex, reset, run

# Keil AC6: separate build dir, CMAKE_TOOLCHAIN_FILE is cached
cmake -G Ninja -S . -B build-ac6 -DXMC_TOOLCHAIN=armclang
ninja -C build-ac6
```

Pico-style manual flow:

```bash
mkdir -p build && cd build
cmake -G Ninja ..
ninja
ninja flash
```

Toolchain roots default to `D:/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi`
and `D:/Keil_v5/ARM/ARMCLANG`; override with `-DARM_GCC_ROOT=…` /
`-DARMCLANG_ROOT=…`. Flashing uses `XMC4500-1024` over SWD at 4000 kHz
(`-DJLINK_DEVICE=…`, `-DJLINK_IF=…`, `-DJLINK_SPEED=…`).

See the [board README](../../README.md) for hardware, clock tree and console
pins.
