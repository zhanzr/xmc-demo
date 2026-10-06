# `st7735_md096_160x80` — ST7735S 0.96" 160x80 LCD on magsensor-2go

Drives the **MD096** 0.96" **160x80** module (**ST7735S** controller) on the
**magsensor-2go** board (XMC1100-Q024x0064, VQFN24) over a 4-wire SPI bus, with
two interchangeable driving methods:

- **SW** — bit-banged GPIO SPI (mode 3);
- **HW** — USIC0 channel 1 SPI master (mode 3), up to ~16 MHz.

The two render the same pattern set alternately so they can be compared on the
panel.

## Wiring

| LCD pin | MCU pin | Role                                          |
| ------- | ------- | --------------------------------------------- |
| SCL     | P0.8    | SPI clock (SW GPIO / USIC0_CH1 SCLKOUT)       |
| SDA     | P0.7    | SPI data out (SW GPIO / USIC0_CH1 DOUT0)      |
| CS      | P0.9    | chip select, GPIO                             |
| DC      | P0.5    | data/command, GPIO                            |
| RES     | P0.6    | reset, GPIO                                   |
| BL      | P2.0    | backlight (CCU40 slice-0 PWM)                 |
| MISO    | P2.6    | left unconnected (input-only pad)             |

`P2.0` is `STD_INOUT/AN` and needs its `PORT2->PDISC` bit cleared, which
`lcd_io_init_out()` / `lcd_io_bind_af()` does.

## Panel geometry

The ST7735S GRAM is 132x162 while the 0.96" glass shows 80x160; the driver
renders **160 wide x 80 tall** in landscape via `MADCTL = 0x60` (row/col
exchange + BGR) with the window seated at `COL_Pre = 0`, `ROW_Pre = 24` (see
`src/lcd.h`). 0.96" modules differ in their offsets — if the picture is shifted
or a strip wraps around one edge, adjust `COL_Pre` / `ROW_Pre` (or toggle
`ST7735_MADCTL` / `ST7735_INVERT_COLORS`).

## What it does

Loops forever, alternating the **SW** and **HW** buses:

1. **Banner** (6x12 font): board/panel name plus the active bus and backlight duty.
2. **Info page**: compiler, build date, driving method (with the HW SPI clock),
   the IO map, backlight duty and chip ID.
3. **Quadrant check**: red/green/blue/white quarters with a black centre cross.
4. **HSV gradient** sweeping left-to-right with a live FPS counter.
5. **LED test**: board LED1 on/off.

Every phase also logs to the console (J-Link VCP, 115200 8-N-1).

## Backlight

`P2.0` is CCU40 slice-0 PWM (~100 kHz). `Backlight_Init()` un-gates the CCU40
peripheral clock in the SCU and starts the slice; `Backlight_SetDuty(percent)`
sets the duty (the demo uses 10 % on the SW bus and 11 % on the HW bus so the
switch is visible). The CCU40 clock gate **must** be cleared or the slice never
runs and the output sits at the passive level.

## HW SPI notes

USIC0 channel 1, SPI master, mode 3. Two details are essential (see
`src/interface.c`):

- the FDR must use the **fractional** divider — the normal divider mode leaves
  the protocol pre-processor unclocked on this part;
- only DX0 (data) may use `INSW = 1`; DX1 (SCLK) and DX2 (MSLS) must take the
  protocol pre-processor outputs (`INSW = 0`), otherwise the shift unit is
  never clocked.

## Build / flash

```bash
bash build.sh                  # gcc (default) -> build/
ninja -C build flash           # J-Link (SWD): program .hex, reset, run
```

Open the J-Link VCP COM port at **115200 8N1** and reset the board.

## Files

- `src/main.c` — pattern set, dual-bus loop
- `src/lcd.c` / `lcd.h` — ST7735S init + 160x80 landscape geometry + drawing/text API
- `src/lcd/lcd_fonts.c` — ASCII 6x12 font (`lcd_font_1608.c` holds an 8x16 font, not built)
- `src/interface.c` / `interface.h` — soft + USIC0_CH1 hardware 4-wire SPI
- `src/blockwrite/blockwrite.h` — pixel-window helper
- `src/backlight.c` / `backlight.h` — P2.0 CCU40 PWM backlight
