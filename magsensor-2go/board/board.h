/**
 * @file    board.h
 * @brief   Board support for the "magsensor-2go" board (XMC1100-Q024x0064).
 *
 * Board-level facts (see magsensor-2go/README.md):
 *   LED1 = P1.0, LED2 = P1.1, both active high
 *   Console = USIC0 channel 0, TXD = P2.1, RXD = P2.2 (J-Link-Lite VCP)
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
 * Safe to call once from main(); the startup file has already configured the
 * flash wait states and the clock tree by then.
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

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
