/**
 * @file    epdif.h
 * @brief   E-paper panel hardware interface for the xmc4500-relax-lite board.
 *
 * USIC2 channel 0 hardware SPI, mode 0 (CPOL = 0, CPHA = 0), MSB first, 8-bit,
 * 2 MHz, write-only. CS is held low for the whole session (a per-byte software
 * CS races the USIC shift register and drops bytes).
 *
 * Wiring (X2 header, see ../../README.md; matches the DAVE EINK reference):
 *   SCK  P5.2   U2C0.SCLKOUT (ALT1)
 *   MOSI P5.0   U2C0.DOUT0   (ALT1)
 *   CS   P2.6   GPIO output (held low)
 *   DC   P5.7   GPIO output
 *   RES  P3.3   GPIO output
 *   BUSY P3.4   GPIO input
 */
#ifndef EPDIF_H
#define EPDIF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    EPDIF_PIN_RST = 0,
    EPDIF_PIN_DC,
    EPDIF_PIN_CS,
    EPDIF_PIN_BUSY
} epdif_pin_t;

/** @brief Bring up the SPI channel and the panel control pins. */
void epdif_init(void);

/** @brief Drive a control pin (RST/DC) high (1) or low (0). BUSY is input only. */
void epdif_write_pin(epdif_pin_t pin, int value);

/** @brief Read a pin level (only EPDIF_PIN_BUSY is meaningful). */
int epdif_read_pin(epdif_pin_t pin);

/**
 * @brief Read BUSY with the internal pull-up enabled.
 *
 * A powered, idle panel drives BUSY low (reads 0); a disconnected BUSY input
 * floats up to 1.
 */
int epdif_busy_pullup_read(void);

/** @brief Blocking delay in milliseconds. */
void epdif_delay_ms(uint32_t ms);

/** @brief Clock one byte out on MOSI (MSB first). */
void epdif_spi_transfer(uint8_t data);

/** @brief Actual SPI shift clock in Hz, computed from the USIC FDR/BRG. */
uint32_t epdif_spi_hz(void);

#ifdef __cplusplus
}
#endif

#endif /* EPDIF_H */
