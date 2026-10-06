/* Shim for compiling with Keil armclang (ARMCLIB-standardlib mode).
 *
 * armclang specializes calls to the *exact name* `printf` into the ARMCLIB
 * ABI: it emits a call to `__2printf` plus hidden references to the internal
 * `_printf_c`/`_printf_d`/... support functions. Those symbols only exist in
 * ARMCLIB, so a GNU-ld + newlib link fails with "unknown destination type
 * (ARM/Thumb)".
 *
 * Renaming `printf` to `bench_printf` stops armclang from recognizing it, so
 * it emits a plain variadic call. `bench_printf` is implemented in
 * board/uart_printf.c as a thin wrapper around newlib `vprintf` (which
 * armclang does NOT specialize), keeping printf() working on the UART.
 */
#ifndef PRINTF_RENAME_H
#define PRINTF_RENAME_H

#define printf bench_printf

int bench_printf(const char *fmt, ...);

#endif /* PRINTF_RENAME_H */
