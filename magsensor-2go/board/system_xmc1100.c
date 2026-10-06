/**
 * @file    system_xmc1100.c
 * @brief   Clock tree bring-up for the XMC1100-Q024x0064.
 *
 * Runs before main() from the DFP startup file:
 *   - one flash wait state (XMC1100 datasheet addendum)
 *   - MCLK = 32 MHz, PCLK = 64 MHz (hardware-fixed PCLK = 2 x MCLK on XMC1)
 */

#include "system_xmc1100.h"

#include "XMC1100.h"

/* SCU_GENERAL->PASSWD values: disable / enable write protection of PV registers. */
#define SCU_PASSWD_UNLOCK (0x000000C0UL)
#define SCU_PASSWD_LOCK   (0x000000C3UL)

/*
 * IDIV = 1, FDIV = 0 -> fractional divider bypassed -> MCLK = DCO1 / 2 = 32 MHz.
 * This is the same CLKCR value Infineon ships in system_XMC1100.c.
 */
#define XMC1100_CLKCR_MCLK_32MHZ_PCLK_64MHZ (0x3FF10100UL)

/*
 * SystemCoreClock lives in .no_init (top of SRAM) so the reset path does not have
 * to copy it from flash. The linker script reserves no_init_size = 4 bytes there.
 */
uint32_t SystemCoreClock __attribute__((section(".no_init")));

void SystemInit(void)
{
    /* Fixed one flash wait state (datasheet addendum); FIXWS freezes the setting. */
    NVM->NVMCONF |= NVM_NVMCONF_WS_Msk;
    NVM->CONFIG1 |= NVM_CONFIG1_FIXWS_Msk;

    SCU_GENERAL->PASSWD = SCU_PASSWD_UNLOCK;
    SCU_CLK->CLKCR = XMC1100_CLKCR_MCLK_32MHZ_PCLK_64MHZ;
    while ((SCU_CLK->CLKCR & SCU_CLK_CLKCR_VDDC2LOW_Msk) != 0UL)
    {
        /* wait for the lower VDDC2 threshold to be released */
    }
    SCU_GENERAL->PASSWD = SCU_PASSWD_LOCK;

    SystemCoreClockUpdate();
}

void SystemCoreClockUpdate(void)
{
    uint32_t idiv;
    uint32_t fdiv;

    idiv = (SCU_CLK->CLKCR & SCU_CLK_CLKCR_IDIV_Msk) >> SCU_CLK_CLKCR_IDIV_Pos;
    fdiv = (SCU_CLK->CLKCR & SCU_CLK_CLKCR_FDIV_Msk) >> SCU_CLK_CLKCR_FDIV_Pos;

    if (idiv != 0UL)
    {
        /* Fractional divider active: MCLK = (DCO1 << 6) / (IDIV << 8 | FDIV) << 1 */
        SystemCoreClock = ((XMC1100_DCO1_HZ << 6U) / ((idiv << 8U) + fdiv)) << 1U;
    }
    else
    {
        /* Fractional divider bypassed: MCLK = DCO1 / 2 */
        SystemCoreClock = XMC1100_DCO1_HZ >> 1U;
    }
}

uint32_t board_cpu_frequency_hz(void)
{
    return SystemCoreClock;
}

uint32_t board_peripheral_clock_hz(void)
{
    return SystemCoreClock * 2UL;
}
