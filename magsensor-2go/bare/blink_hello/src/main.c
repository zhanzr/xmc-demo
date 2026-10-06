/**
 * @file    main.c
 * @brief   magsensor-2go blink_hello: clock report, LED blink, DTS temperature.
 *
 * The XMC1100 has no internal VADC channels (reference manual Table 15-4 lists
 * only external P2.x pads), so the periodic "internal analog" reading is the
 * on-die temperature sensor (DTS) exposed through SCU_ANALOG->ANATSEMON.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "board.h"
#include "system_xmc1100.h"
#include "tse.h"

#define BLINK_INTERVAL_MS (500UL)
#define REPORT_INTERVAL_MS (1000UL)

static void print_banner(void)
{
    printf("\r\n");
    printf("magsensor-2go / XMC1100-Q024x0064 / blink_hello\r\n");
    printf("  CPU clock (MCLK)   : %lu Hz (%lu.%03lu MHz)\r\n",
           (unsigned long)board_cpu_frequency_hz(),
           (unsigned long)(board_cpu_frequency_hz() / 1000000UL),
           (unsigned long)((board_cpu_frequency_hz() / 1000UL) % 1000UL));
    printf("  Peripheral (PCLK)  : %lu Hz (%lu.%03lu MHz)\r\n",
           (unsigned long)board_peripheral_clock_hz(),
           (unsigned long)(board_peripheral_clock_hz() / 1000000UL),
           (unsigned long)((board_peripheral_clock_hz() / 1000UL) % 1000UL));
    printf("  Console            : USIC0_CH0 TXD=P2.1 RXD=P2.2, 115200 8N1\r\n");
    printf("  LEDs               : LED1=P1.0 LED2=P1.1 (active high)\r\n");
    printf("\r\n");
}

int main(void)
{
    uint32_t next_blink;
    uint32_t next_report;

    board_init();
    tse_init();
    print_banner();

    next_blink = board_millis();
    next_report = board_millis();

    for (;;)
    {
        uint32_t now = board_millis();

        if ((int32_t)(now - next_blink) >= 0)
        {
            board_led_toggle(BOARD_LED_1);
            board_led_toggle(BOARD_LED_2);
            next_blink = now + BLINK_INTERVAL_MS;
        }

        if ((int32_t)(now - next_report) >= 0)
        {
            int32_t millidegree = tse_read_millidegree_c();
            uint32_t mclk = board_cpu_frequency_hz();

            printf("[%8lu ms] DTS: %lu.%03lu C  (CPU/MCLK %lu.%03lu MHz)\r\n",
                   (unsigned long)now,
                   (unsigned long)((uint32_t)millidegree / 1000UL),
                   (unsigned long)((uint32_t)millidegree % 1000UL),
                   (unsigned long)(mclk / 1000000UL),
                   (unsigned long)((mclk / 1000UL) % 1000UL));

            next_report = now + REPORT_INTERVAL_MS;
        }

        __asm volatile("wfi");
    }
}
