/**
 * @file    main.c
 * @brief   xmc4500-relax-lite blink_hello: clock report, LED blink, DTS and EVR.
 *
 * The XMC4500 has an on-die temperature sensor (DTS) and the EVR13/EVR33 supply
 * monitors, both read through the vendored XMClib SCU driver.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "board.h"
#include "xmc_scu.h"

#define BLINK_INTERVAL_MS (500UL)
#define REPORT_INTERVAL_MS (1000UL)

/* Die temperature, XMCLib formula (xmc4_scu.h): T[degC] = (RESULT - 605) / 2.05. */
#define DTS_OFFSET    (605)
#define DTS_MILLI_DIV (2050) /* 2.05 * 1000, keeps the maths integer */
#define DTS_MIN_MILLI (-40000) /* -40 C */
#define DTS_MAX_MILLI (125000) /* 125 C */

static int32_t read_temperature_millidegc(void)
{
    uint32_t raw;
    int32_t millideg;

    while (XMC_SCU_IsTemperatureSensorReady() == false)
    {
        /* wait for the running conversion */
    }
    raw = XMC_SCU_GetTemperatureMeasurement();

    /* Start the next conversion for the following report. */
    (void)XMC_SCU_StartTemperatureMeasurement();

    millideg = ((int32_t)raw - DTS_OFFSET) * 1000000;
    millideg = millideg / DTS_MILLI_DIV;

    /* The first conversion after enabling the sensor can be garbage; gate it. */
    if ((millideg < DTS_MIN_MILLI) || (millideg > DTS_MAX_MILLI))
    {
        return 25000; /* 25.000 C fallback */
    }

    return millideg;
}

static void print_banner(void)
{
    uint32_t cpu = board_cpu_frequency_hz();
    uint32_t pclk = board_peripheral_clock_hz();

    printf("\r\n");
    printf("xmc4500-relax-lite / XMC4500-F100x1024 / blink_hello\r\n");
    printf("  CPU clock (fCPU)   : %lu Hz (%lu.%03lu MHz)\r\n",
           (unsigned long)cpu,
           (unsigned long)(cpu / 1000000UL),
           (unsigned long)((cpu / 1000UL) % 1000UL));
    printf("  Peripheral (fPB)   : %lu Hz (%lu.%03lu MHz)\r\n",
           (unsigned long)pclk,
           (unsigned long)(pclk / 1000000UL),
           (unsigned long)((pclk / 1000UL) % 1000UL));
    printf("  Console            : USIC1_CH0 TXD=P0.5 RXD=P0.4, 115200 8N1\r\n");
    printf("  LEDs               : LED1=P1.0 LED2=P1.1 (active high)\r\n");
    printf("\r\n");
}

int main(void)
{
    uint32_t next_blink;
    uint32_t next_report;

    board_init();

    XMC_SCU_EnableTemperatureSensor();
    (void)XMC_SCU_StartTemperatureMeasurement();
    /* Discard the first, unstable conversion so reports are meaningful. */
    while (XMC_SCU_IsTemperatureSensorReady() == false)
    {
    }
    (void)XMC_SCU_GetTemperatureMeasurement();
    (void)XMC_SCU_StartTemperatureMeasurement();

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
            int32_t temp = read_temperature_millidegc();
            uint32_t evr13 = (uint32_t)(XMC_SCU_POWER_GetEVR13Voltage() * 1000.0f);
            uint32_t evr33 = (uint32_t)(XMC_SCU_POWER_GetEVR33Voltage() * 1000.0f);

            printf("[%8lu ms] DTS: %lu.%03lu C  EVR13: %lu mV  EVR33: %lu mV\r\n",
                   (unsigned long)now,
                   (unsigned long)((uint32_t)temp / 1000UL),
                   (unsigned long)((uint32_t)temp % 1000UL),
                   (unsigned long)evr13,
                   (unsigned long)evr33);

            next_report = now + REPORT_INTERVAL_MS;
        }

        __asm volatile("wfi");
    }
}
