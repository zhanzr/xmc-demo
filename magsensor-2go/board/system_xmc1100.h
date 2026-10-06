/**
 * @file    system_xmc1100.h
 * @brief   Clock tree bring-up and frequency reporting for the XMC1100-Q024x0064.
 *
 * Called from `Reset_Handler` (startup_XMC1100.S) before main() runs.
 */
#ifndef SYSTEM_XMC1100_H
#define SYSTEM_XMC1100_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** DCO1 oscillator frequency. MCLK is DCO1/2 unless the fractional divider runs. */
#define XMC1100_DCO1_HZ (64000000UL)

/** Cortex-M0+ system clock (MCLK) in Hz. Updated by SystemCoreClockUpdate(). */
extern uint32_t SystemCoreClock;

void SystemInit(void);
void SystemCoreClockUpdate(void);

/** MCLK in Hz (the Cortex-M0 core clock). */
uint32_t board_cpu_frequency_hz(void);

/** PCLK in Hz (AHB/APB peripheral clock). Fixed at 2 x MCLK on XMC1. */
uint32_t board_peripheral_clock_hz(void);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_XMC1100_H */
