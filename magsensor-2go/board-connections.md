# magsensor-2go — board connections

Pin-level connections for the **magsensor-2go** board (Infineon
**XMC1100-Q024x0064**, VQFN24).

Most of this matches the board document / manual as published. Two entries in
that document are **wrong** and are corrected below — everything else in it was
found to be correct.

## Corrections to the published board document

| Item                | Document says        | Correction                                  |
| ------------------- | -------------------- | ------------------------------------------- |
| LED1                | `P0.12`              | **`P1.0`** — LED1 is on P1.0, not P0.12     |
| Onboard sensor VDD  | driven by `P1.0`     | **board power rail** — not connected to a GPIO at all |

Both were verified on hardware:

* `LED1`/`LED2` blink on `P1.0`/`P1.1` in every project here (`blink_hello`,
  `sweep_io`); `P0.12` drives nothing on this board.
* `P1.0` drives LED1 only — it is **not** the sensor's VDD, so it is free of any
  power-control duty and must not be repurposed for one.

## Verified pin connections

| Net                    | Pin                  | Notes                                                    |
| ---------------------- | -------------------- | -------------------------------------------------------- |
| LED1                   | `P1.0`               | active high                                              |
| LED2                   | `P1.1`               | active high                                              |
| Console TXD            | `P2.1`               | USIC0 channel 0, ALT6, 115200 8N1 → J-Link-Lite VCP      |
| Console RXD            | `P2.2`               | USIC0 channel 0 input (DX3), `PORT2->PDISC` cleared first |
| SWCLK                  | `P1.2`               | J-Link SWD, board manual                                 |
| SWDIO                  | `P1.3`               | J-Link SWD, board manual                                 |
| Sensor VDD             | board power rail     | **not** a GPIO (see corrections above)                   |

Chip identity: `SCU_GENERAL->IDCHIP` and flash `0x10000F00` both read
`0x00011062` = **XMC1100-Q024F0064**, which confirms the `-Q024` package code and
the VQFN24 footprint.

## Pad capabilities and safe use

Pad types are from datasheet Table 6 (XMC1000/XMC1100, V1.8, 2016-09).

| Pads                                                     | Type          | Usable as              |
| -------------------------------------------------------- | ------------- | ---------------------- |
| `P0.0`, `P0.5`–`P0.9`, `P0.12`–`P0.15`, `P1.0`–`P1.6`     | `STD_INOUT`   | input or output        |
| `P2.0`, `P2.1`, `P2.10`, `P2.11`                          | `STD_INOUT/AN`| input, output or analog |
| `P2.2`, `P2.6`–`P2.9` (bonded) | `STD_IN/AN`   | **input or analog only — no output driver** |

Not bonded on VQFN24 (Table 6): `P0.1`–`P0.4`, `P0.10`, `P0.11`, `P1.4`–`P1.6`,
and `P2.3`–`P2.5`. The bonded pads are exactly `P0.0`, `P0.5`–`P0.9`,
`P0.12`–`P0.15`, `P1.0`–`P1.3`, and `P2.0`–`P2.2`, `P2.6`–`P2.11`.
`P2.7` and `P2.8` are the same physical pad (5) on this package.

The "no output driver" claim for `P2.2`–`P2.9` was measured directly: with
`PORT2->OUT` set, `PORT2->IN` stays low on `P2.6`, `P2.7`/`P2.8` and `P2.9`
(`OUT=1 IN=0`). See `bare/sweep_io`.

## Hardware traps

* Keep `P1.2` (SWCLK) and `P1.3` (SWDIO) out of any pin sweep, or J-Link stops
  responding until the board is reset.
* `P2.1`/`P2.2` carry the console; driving them garbles or loses output.
* `PORT0`/`PORT1` `PDISC` is **read-only** — writing it raises a HardFault (the
  DFP header declares it `__I`). Only `PORT2` has analog-capable pads, so only
  `PORT2->PDISC` needs clearing, and it resets to `0xFFFF`.

See [`bare/sweep_io`](bare/sweep_io) for the sweep program used to establish
these.