/**
 * @file    console.h
 * @brief   Polled UART console on USIC1 channel 0 (TXD = P0.5, RXD = P0.4).
 *
 * On the "xmc4500-relax-lite" board these two pads are wired to the external
 * COM bridge, so the console shows up as that bridge's COM port. Settings:
 * 115200 baud, 8 data bits, no parity, 1 stop bit (8N1).
 */
#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Baud rate used by board_init(). */
#define CONSOLE_DEFAULT_BAUD (115200UL)

/** Bring up USIC1 channel 0, P0.5 (TXD) and P0.4 (RXD). */
void console_init(uint32_t baud);

/** Queue one character (blocking until the transmit buffer accepts it). */
void console_putc(char c);

/** Queue @p len characters. */
void console_write(const char *data, size_t len);

/** Queue a NUL-terminated string. */
void console_puts(const char *s);

/**
 * @brief Read up to @p len characters, stopping early at CR, LF or a timeout.
 *
 * CR and LF are not stored; at most one of them is consumed. Returns the number
 * of characters stored in @p buf (which is always NUL-terminated).
 */
size_t console_readline(char *buf, size_t len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* CONSOLE_H */
