/**
 * @file    board.h
 * @brief   Board support for the "xmc4500-relax-lite" board (XMC4500-F100x1024).
 *
 * Board-level facts (see xmc4500-relax-lite/README.md):
 *   LED1 = P1.0, LED2 = P1.1, both active high
 *   Console = USIC1 channel 0, TXD = P0.5, RXD = P0.4 (external COM bridge)
 */
#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Number of user LEDs on the board. */
#define BOARD_LED_COUNT (2U)

#define BOARD_LED_1 (0U)
#define BOARD_LED_2 (1U)

/**
 * @brief Initialise clocks, LEDs, the SysTick millisecond tick and the console.
 *
 * Safe to call once from main(); the DFP startup file has already run
 * SystemInit() and configured the clock tree by then.
 */
void board_init(void);

/** @brief Free-running millisecond counter since board_init(). */
uint32_t board_millis(void);

/** @brief Block for @p ms milliseconds. */
void board_delay_ms(uint32_t ms);

/** @brief Drive LED @p led (BOARD_LED_1 / BOARD_LED_2) on or off. Active high. */
void board_led_set(uint32_t led, bool on);

/** @brief Toggle LED @p led. */
void board_led_toggle(uint32_t led);

/** @brief CPU (fCPU) clock in Hz. */
uint32_t board_cpu_frequency_hz(void);

/** @brief Peripheral bus (fPB) clock in Hz. */
uint32_t board_peripheral_clock_hz(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
