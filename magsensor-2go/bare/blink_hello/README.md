# blink_hello (magsensor-2go, XMC1100)

Smoke test / bring-up project for the **magsensor-2go** board: brings up the board
layer, prints the live CPU and peripheral clocks over the console, blinks both
LEDs and reports the die temperature once a second.

## What it does

* `board_init()` → clocks (already set by `SystemInit()`), LEDs, SysTick and the
  USIC0 channel 0 console.
* prints the banner (MCLK/PCLK, console pins, LED pins),
* toggles LED1 (`P1.0`) and LED2 (`P1.1`) every **500 ms** (active high),
* every **1000 ms** prints the DTS reading and the current CPU (MCLK) frequency,
  both read from the hardware at that moment — the clock is derived from
  `SCU_CLK->CLKCR` (IDIV/FDIV), not a constant.

The XMC1100 has no internal VADC channels (reference manual Table 15-4 lists only
external `P2.x` pads), so the periodic "internal analog" reading is the on-die
**temperature sensor (DTS)** in `SCU_ANALOG->ANATSEMON`, offset-corrected from the
OTP factory calibration values (`board/tse.c`).

## Expected console output

J-Link VCP at **115200 8N1** (TXD `P2.1`, RXD `P2.2`):

```text
magsensor-2go blink_hello: clock report, LED blink, DTS temperature.
CPU/MCLK: 32000000 Hz (32.000 MHz)
PCLK    : 64000000 Hz (64.000 MHz)
Console : USIC0_CH0 TXD=P2.1 RXD=P2.2 115200 8N1 (J-Link VCP)
LEDs    : LED1=P1.0 LED2=P1.1 (active high)
[      34 ms] DTS: 35.000 C  (CPU/MCLK 32.000 MHz)
[    1034 ms] DTS: 35.000 C  (CPU/MCLK 32.000 MHz)
[    2034 ms] DTS: 35.000 C  (CPU/MCLK 32.000 MHz)
```

Typical room-temperature reading is 33–36 °C; the value is a die temperature, so
it runs a few degrees above ambient.

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
`-DARMCLANG_ROOT=…`. Flashing uses `XMC1100-0064` over SWD at 4000 kHz
(`-DJLINK_DEVICE=…`, `-DJLINK_IF=…`, `-DJLINK_SPEED=…`).

See the [board README](../../README.md) for hardware, clock tree and console
pins.