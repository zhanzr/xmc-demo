# eink_154 (xmc4500-relax-lite, XMC4500)

1.54" 200x200 monochrome e-paper test for the **xmc4500-relax-lite** board,
driving a LuatOS **Eink-1.54** module.

Controller: **SSD1608** ("V1"), 200x200, **2-level** (black/white only — the
vendor's "Grey level: 2" means two levels, not four). The st7735 bring-up page
set is reproduced here in black/white, plus an info page that reports the die
temperature and the EVR13/EVR33 supply monitors.

## Wiring (X2 header)

| Panel pin | Signal | XMC4500 pin | Function                    |
| --------- | ------ | ----------- | --------------------------- |
| VCC       | 3V3    | V33 (X2)    | 3.3 V supply (do not use 5 V) |
| GND       | GND    | GND (X2)    | ground                      |
| DIN       | MOSI   | `P5.0`      | `U2C0.DOUT0` (ALT1)         |
| CLK       | SCK    | `P5.2`      | `U2C0.SCLKOUT` (ALT1)       |
| CS        | CS     | `P2.6`      | GPIO output (held low)      |
| DC        | DC     | `P5.7`      | GPIO output                 |
| RST       | RES    | `P3.3`      | GPIO output                 |
| BUSY      | BUSY   | `P3.4`      | GPIO input                  |

USIC2 channel 0 hardware SPI, **mode 0** (CPOL = 0, CPHA = 0), MSB first,
8-bit, 2 MHz, write-only. Matches the DAVE reference project
`D:\dave_wks\xmc4500_lqfp100` (`SPI_MASTER` USIC2_CH0 + `DIGITAL_IO`
`EINK_RES`/`EINK_DC`/`EINK_BUSY`).

## Page set (looping)

| Page       | Content                                                              |
| ---------- | ------------------------------------------------------------------- |
| banner     | title text in a double border                                       |
| info       | compiler/pins + **DTS**, **EVR13**, **EVR33** and CPU clock         |
| quadrant   | solid black / solid white / checkerboard / 25 % dither + centre cross |
| gradient   | horizontal 4x4-Bayer dithered black→white ramp                      |
| LED        | "LED on" / "LED off" pages with both LEDs toggling                  |

Each page re-initialises the panel and refreshes (a full update takes ~2 s).

## Expected console output

External COM bridge (**COM93**) at **115200 8N1** (TXD `P0.5`, RXD `P0.4`):

```text
==== xmc4500-relax-lite eink_154 (SSD1608 200x200) ====
USIC2_CH0 HW SPI: SCK=P5.2 MOSI=P5.0 CS=P2.6 DC=P5.7 RES=P3.3 BUSY=P3.4
[epd] BUSY probe (pull-up) = 0
[epd] page: banner
[epd] page: info
[epd] page: quadrant
[epd] page: gradient
[epd] page: LED on
[epd] page: LED off
```

## Build / flash

```bash
bash build.sh                 # gcc (arm-none-eabi), output in build/
ninja -C build flash          # J-Link (SWD): program .hex, reset, run

# Keil AC6: separate build dir, CMAKE_TOOLCHAIN_FILE is cached
cmake -G Ninja -S . -B build-ac6 -DXMC_TOOLCHAIN=armclang
ninja -C build-ac6
```

## Source layout

```
src/
├── main.c        page set (banner / info / quadrant / gradient / LED)
├── canvas.c/.h   monochrome 200x200 framebuffer, shapes + 5x8 text
├── font8.c       ST 5x8 column-major font table (+ fonts.h)
├── epd1in54.c/.h SSD1608/SSD1681 driver
└── epdif.c/.h    USIC2_CH0 SPI + CS/DC/RES/BUSY
```

## Notes

* **USIC2 is held in the SCU peripheral-reset domain**: release it with
  `SCU_RESET->PRCLR1 = SCU_RESET_PRSTAT1_USIC2RS_Msk` (`epdif.c`), same trap as
  USIC1 for the console.
* **`P2.6` is an analog-capable pad**: clear its `PDISC` bit (done in
  `epdif.c`), because `XMC_GPIO_SetMode` does not.
* **Keep CS low for the whole session.** A per-byte software CS races the USIC
  shift register and drops bytes — on hardware that produced a sheared "zebra"
  image. Hardware SPI with CS held low renders cleanly.
* The driver keeps an `EPD_VARIANT_V2` (SSD1681) path and a BUSY-pulse based
  bring-up sweep history; this module is V1/SSD1608 with update command `0xC4`.
* The die-temperature formula is the XMCLib one,
  `T[degC] = (DTSTAT.RESULT - 605) / 2.05`, evaluated as integer millidegrees.
