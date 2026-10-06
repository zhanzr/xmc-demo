# magsensor-2go — XMC1100-Q024x0064 development projects

Bare-metal projects for the **magsensor-2go** board
(Infineon **XMC1100-Q024x0064**, 32-pin QFN, Cortex-M0).

![magsensor-2go board](board_images/board_1.png)

The larger photos (`board_0.png`, `board_2.png`) are not embedded here — download
them from [`board_images/`](board_images) to view them locally.

## Board facts

| Item                | Value                                                        |
| ------------------- | ------------------------------------------------------------ |
| MCU                 | Infineon XMC1100-Q024x0064 (Cortex-M0, 32-pin QFN)          |
| Flash               | 64 KB at `0x10001000` (sector 0 is the boot/config area)     |
| SRAM                | 16 KB at `0x20000000`, no heap (stack = `BOARD_STACK_SIZE`, 3 KB default) |
| Clock               | DCO1 = 64 MHz → MCLK = 32 MHz, PCLK = 64 MHz, 1 flash wait state |
| LED1 / LED2         | `P1.0` / `P1.1`, **active high**                              |
| Console             | USIC0 channel 0, TXD = `P2.1` (ALT6), RXD = `P2.2`, 115200 8N1 |
| Debug / programming | J-Link (SWD); console on the J-Link-Lite VCP COM port        |
| Die temperature     | `SCU_ANALOG->ANATSEMON`, OTP-calibrated (see `board/tse.c`)  |

The console pins are also where the DFP's own `XMC_GPIO_Init()`-style pad setup
would leave the pads **disabled** (`PORT2->PDISC` resets to `0xFFFF`), so
`board/console.c` explicitly clears the `P2.1`/`P2.2` bits before enabling the
alternate function.

## Clock tree (32 MHz MCLK / 64 MHz PCLK)

`board/system_xmc1100.c` runs from the DFP startup file *before* `main()`:

1. one flash wait state (`NVM->NVMCONF` + `FIXWS`),
2. `SCU_CLK->CLKCR = 0x3FF10100` (IDIV = 1, FDIV = 0 → fractional divider
   bypassed → MCLK = DCO1/2 = 32 MHz; PCLK is hardware-fixed at 2 × MCLK = 64 MHz
   on XMC1),
3. `SysTick` is clocked from MCLK by `board_init()`.

`board_cpu_frequency_hz()` / `board_peripheral_clock_hz()` return the values
derived from `SCU_CLK->CLKCR`, so applications print the real runtime clocks
rather than a hard-coded number.

## Projects (`bare/`)

| Project                                | What it does                                                        |
| -------------------------------------- | ------------------------------------------------------------------- |
| [`bare/blink_hello`](bare/blink_hello) | LED blink + die-temperature (DTS) report over the console, with the live CPU/MCLK frequency |

`bare/` holds projects that use the board layer directly — no RTOS, no HAL above
`board/`. See [`bare/README.md`](bare/README.md).

## Build / flash / serial console

Toolchains: GNU `arm-none-eabi-gcc` (default) or Keil AC6 `armclang`
(`-DXMC_TOOLCHAIN=armclang`). Device + CMSIS headers come from the repo-root
`drivers/` (see the root README).

```bash
cd bare/blink_hello

bash build.sh                 # configure + build with gcc into build/
ninja -C build flash          # program through J-Link (SWD), then reset+run

# Keil AC6 (separate build dir: CMAKE_TOOLCHAIN_FILE is cached)
cmake -G Ninja -S . -B build-ac6 -DXMC_TOOLCHAIN=armclang
ninja -C build-ac6
```

Then open the J-Link VCP COM port at **115200 8N1** and reset the board. The
`blink_hello` banner and its 1 Hz DTS report appear on the console, and both LEDs
blink at 2 Hz:

```text
magsensor-2go blink_hello: clock report, LED blink, DTS temperature.
CPU/MCLK: 32000000 Hz (32.000 MHz)
PCLK    : 64000000 Hz (64.000 MHz)
Console : USIC0_CH0 TXD=P2.1 RXD=P2.2 115200 8N1 (J-Link VCP)
LEDs    : LED1=P1.0 LED2=P1.1 (active high)
[      34 ms] DTS: 35.000 C  (CPU/MCLK 32.000 MHz)
[    1034 ms] DTS: 35.000 C  (CPU/MCLK 32.000 MHz)
```

Other useful targets: `ninja flash-bin` (raw `.bin` at `0x10001000`),
`ninja erase`. J-Link settings can be overridden with `-DJLINK_DEVICE=…`,
`-DJLINK_IF=…`, `-DJLINK_SPEED=…`.