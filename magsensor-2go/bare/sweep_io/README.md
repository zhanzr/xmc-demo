# `sweep_io` — GPIO discovery sweep

Announces every pad that can be driven on the XMC1100-Q024x0064 (**VQFN24**) and
toggles that pin on its own, so the actual board wiring can be identified by
watching it. Use it to confirm which pins the LEDs, sensors and headers really
use before trusting the board documentation.

## Expected output

```
magsensor-2go / XMC1100 / sweep_io
  SCU_GENERAL->IDCHIP : 0x00011062  [Q024F0064 as expected]
  flash 0x10000F00   : 0x00011062  [Q024F0064 as expected]
  package code       : -Q024 (24-pin VQFN)
  package            : VQFN24
  console (excluded) : P2.1 TXD, P2.2 RXD
  SWD (excluded)     : P1.2 SWCLK, P1.3 SWDIO
  pins to sweep      : 18 (3 of them input-only)
  timing             : 200 ms per toggle, 3000 ms per pin

  P0.0      (pad 15) done
  P0.5      (pad 16) done
  ...
```

`0x00011062` is the chip ID of **XMC1100-Q024F0064** (datasheet Table 4), read
both from `SCU_GENERAL->IDCHIP` and from its copy in flash config sector 0
(`0x10000F00`). It confirms the `-Q024` package code and the 24-pin VQFN
footprint; `-Q040F0032` reads `0x00011042`.

Each pin is announced once, then toggles every **200 ms for 3 s** with no other
console output, then returns to high impedance. The round robin repeats until
reset, one pin driven at a time.

## Pins swept (18)

| Port | Pins                                                                         | VQFN24 pads |
| ---- | ---------------------------------------------------------------------------- | ----------- |
| P0   | P0.0, P0.5, P0.6, P0.7, P0.8, P0.9, P0.12, P0.13, P0.14, P0.15                   | 15–24 |
| P1   | P1.0, P1.1                                                                    | 14, 13 (high-current pads) |
| P2   | P2.0, P2.6, P2.7/P2.8, P2.9, P2.10, P2.11                                     | 1, 4, 5, 6, 7, 8 |

`P2.6`, `P2.7/P2.8` and `P2.9` are logged as `input-only`: they are `STD_IN/AN`
pads on this package (datasheet Table 6), so they have no output driver.
Confirmed on hardware — the log reads back the pad after each pin:

```
  P2.6      (pad  4) input-only done [OUT=1 IN=0: no output driver]
  P2.7/P2.8 (pad  5) input-only done [OUT=1 IN=0: no output driver]
  P2.9      (pad  6) input-only done [OUT=1 IN=0: no output driver]
```

`OUT` is the bit written to `PORT2->OUT`, `IN` is `PORT2->IN` sampled right
after the last toggle. They disagree, i.e. the pad does not follow. So these
three pads cannot be used as outputs; only as inputs or analog inputs. `P2.7`
and `P2.8` are the same physical pad (5) on VQFN24.

## Pins deliberately not swept

| Pin                                        | Reason                                          |
| ------------------------------------------ | ----------------------------------------------- |
| P2.1, P2.2                                 | console UART (TXD/RXD) on the J-Link-Lite VCP   |
| P1.2, P1.3                                 | SWD: SWCLK / SWDIO (board manual)               |
| P0.1–P0.4, P0.10, P0.11, P1.4–P1.6         | not bonded on VQFN24 (datasheet Table 6)        |

Pad numbers come from the datasheet package pin mapping (Table 6) so the log can
be read straight against it.

## Board-layer note

`PORT0`/`PORT1` `PDISC` is read-only: writing it raises a HardFault (the DFP
header marks them `__I`). Only `PORT2` has analog-capable pads, so only `PORT2`
needs its `PDISC` cleared — it resets to `0xFFFF`, i.e. pads disabled. See
`pad_enable()` in `src/main.c`.

## Build / flash

```bash
bash build.sh                  # gcc (default) -> build/
ninja -C build flash           # J-Link (SWD): program .hex, reset, run
ninja -C build erase           # unprogram the board
```

Keil Arm Compiler 6 (separate build dir, `CMAKE_TOOLCHAIN_FILE` is cached):

```bash
cmake -G Ninja -S . -B build-ac6 -DXMC_TOOLCHAIN=armclang
ninja -C build-ac6
ninja -C build-ac6 flash
```

Open the J-Link VCP COM port at **115200 8N1** and reset the board to watch the
sweep. One full round takes 17 × 3 s ≈ 51 s.

J-Link settings can be overridden with `-DJLINK_DEVICE=…`, `-DJLINK_IF=…`,
`-DJLINK_SPEED=…`.