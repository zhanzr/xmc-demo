/**
 * @file    board.c
 * @brief   Board support for the "xmc4500-relax-lite" board (XMC4500-F100x1024).
 */

#include "board.h"

#include <stdio.h>

#include "console.h"
#include "system_XMC4500.h"

#include "XMC4500.h"

/*
 * XMC4 port registers. OUT is at +0x00 and the IOCRn registers start at +0x10;
 * each IOCR holds four 8-bit pin-control lanes (one per pin, 4 pins per register).
 */
#define PORT_OUT_OFFSET   (0x00U)
#define PORT_IOCR0_OFFSET (0x10U)

#define PORT_IOCR_PC_Msk (0xF8UL)

/* Pin-control lane encodings (bits [7:3] of each 8-bit lane). */
#define GPIO_MODE_INPUT_TRISTATE   (0x00UL)
#define GPIO_MODE_OUTPUT_PUSH_PULL (0x80UL)

#define SYSTICK_HZ (1000UL)

/* LED1 = P1.0, LED2 = P1.1 (active high). */
static const uint32_t led_port_index[BOARD_LED_COUNT] = { 1U, 1U };
static const uint32_t led_pin[BOARD_LED_COUNT] = { 0U, 1U };

static volatile uint32_t board_ms_ticks;

static void gpio_set_mode(volatile uint32_t *port, uint32_t pin, uint32_t mode)
{
    volatile uint32_t *iocr;
    uint32_t shift = 8UL * (pin & 3UL);

    iocr = (volatile uint32_t *)((uintptr_t)port + PORT_IOCR0_OFFSET + ((pin >> 2U) * 4UL));
    *iocr = (*iocr & ~(PORT_IOCR_PC_Msk << shift)) | (mode << shift);
}

static void gpio_write(volatile uint32_t *port, uint32_t pin, bool level)
{
    volatile uint32_t *out = (volatile uint32_t *)((uintptr_t)port + PORT_OUT_OFFSET);

    if (level)
    {
        *out |= (1UL << pin);
    }
    else
    {
        *out &= ~(1UL << pin);
    }
}

static volatile uint32_t *gpio_port(uint32_t index)
{
    if (index == 1U)
    {
        return (volatile uint32_t *)PORT1;
    }
    if (index == 2U)
    {
        return (volatile uint32_t *)PORT2;
    }

    return (volatile uint32_t *)PORT0;
}

static void leds_init(void)
{
    uint32_t led;

    for (led = 0U; led < BOARD_LED_COUNT; ++led)
    {
        volatile uint32_t *port = gpio_port(led_port_index[led]);

        gpio_write(port, led_pin[led], false);
        gpio_set_mode(port, led_pin[led], GPIO_MODE_OUTPUT_PUSH_PULL);
    }
}

void board_init(void)
{
    SystemCoreClockUpdate();

    leds_init();

    /* 1 kHz millisecond tick used by board_millis()/board_delay_ms(). */
    SysTick_Config(board_cpu_frequency_hz() / SYSTICK_HZ);
    board_ms_ticks = 0UL;

    console_init(CONSOLE_DEFAULT_BAUD);

    /*
     * Send stdout out immediately. Without this newlib buffers printf output
     * and only reaches _write() when the buffer fills, so nothing would ever
     * reach the USIC transmit register for short messages.
     */
    (void)setvbuf(stdout, NULL, _IONBF, 0);
    (void)setvbuf(stderr, NULL, _IONBF, 0);
}

uint32_t board_millis(void)
{
    return board_ms_ticks;
}

void board_delay_ms(uint32_t ms)
{
    uint32_t start = board_ms_ticks;

    while ((board_ms_ticks - start) < ms)
    {
        __WFI();
    }
}

void board_led_set(uint32_t led, bool on)
{
    if (led < BOARD_LED_COUNT)
    {
        gpio_write(gpio_port(led_port_index[led]), led_pin[led], on);
    }
}

void board_led_toggle(uint32_t led)
{
    if (led < BOARD_LED_COUNT)
    {
        volatile uint32_t *port = gpio_port(led_port_index[led]);
        volatile uint32_t *out = (volatile uint32_t *)((uintptr_t)port + PORT_OUT_OFFSET);

        *out ^= (1UL << led_pin[led]);
    }
}

uint32_t board_cpu_frequency_hz(void)
{
    return SystemCoreClock;
}

uint32_t board_peripheral_clock_hz(void)
{
    uint32_t pdiv = (SCU_CLK->PBCLKCR & SCU_CLK_PBCLKCR_PBDIV_Msk) >> SCU_CLK_PBCLKCR_PBDIV_Pos;

    return SystemCoreClock >> pdiv;
}

void SysTick_Handler(void)
{
    board_ms_ticks++;
}
