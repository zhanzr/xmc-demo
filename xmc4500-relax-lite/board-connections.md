# xmc4500-relax-lite — board connections

Pin-level connections for the **xmc4500-relax-lite** board (Infineon
**XMC4500-F100x1024**, LQFP-100).

## Verified pin connections

| Net         | Pin    | Notes                                                              |
| ----------- | ------ | ------------------------------------------------------------------ |
| LED1        | `P1.0` | active high, push-pull GPIO                                        |
| LED2        | `P1.1` | active high, push-pull GPIO                                        |
| Console TXD | `P0.5` | USIC1 channel 0, ALT2 (`U1C0.DOUT0`), 115200 8N1 → external COM bridge |
| Console RXD | `P0.4` | USIC1 channel 0 input `USIC1_C0_DX0_P0_4`                          |

SWD programming/debug uses the onboard debugger (J-Link); see the board manual
for the SWD pin assignment.

## Pad capabilities and safe use

* `PORT2`, `PORT14` and `PORT15` are the analog-capable ports on XMC4500; their
  `PDISC` registers reset to all-disabled and are handled by XMClib when a pin is
  configured. PORT0/PORT1 (console, LEDs) are digital-only, so no `PDISC` write is
  needed there.
* The console pins `P0.4`/`P0.5` carry the external COM bridge: keep them out of
  any pin sweep, or console output is lost.
* Keep the SWD pins out of any pin sweep, or the debugger stops responding until
  the board is reset.
