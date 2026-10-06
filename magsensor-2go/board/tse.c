/**
 * @file    tse.c
 * @brief   XMC1100 die temperature sensor (TSE) driver.
 *
 * The conversion follows Infineon's reference routine (_CalcTemperature from the
 * XMC1000 "Bootstrap Loaders and User Routines" chapter): a quadratic
 * T(rx) = (k1 - sqrt(k1^2 - 4*k3*(rx - k2))) / (2*k3) evaluated in fixed point
 * with an integer square root, followed by a per-Kelvin correction table stored
 * in flash sector 0.
 */

#include "tse.h"

#include "XMC1100.h"

/*
 * Flash sector 0 is mapped read-only at 0x10000000. It is not part of the linker
 * script's FLASH region, which starts at 0x10001000 (user code). See the XMC1100
 * reference manual, Table 7-2 "XMC1100 Memory Map".
 */
#define TSE_OTP_BASE (0x10000F20UL)

#define TSE_OTP_K1     ((const uint32_t *)(TSE_OTP_BASE + 0x00UL))
#define TSE_OTP_K3     ((const uint32_t *)(TSE_OTP_BASE + 0x04UL))
#define TSE_OTP_K2     ((const uint32_t *)(TSE_OTP_BASE + 0x08UL))
#define TSE_OTP_MON_HI ((const uint16_t *)(TSE_OTP_BASE + 0x0CUL))
#define TSE_OTP_MON_LO ((const uint16_t *)(TSE_OTP_BASE + 0x0EUL))
#define TSE_OTP_CORR   ((const int8_t *)(TSE_OTP_BASE + 0x14UL))

#define TSE_KELVIN_OFFSET (273U)

#define SCU_PASSWD_UNLOCK (0x000000C0UL)
#define SCU_PASSWD_LOCK   (0x000000C3UL)

/** Integer square root, rounding up (Otto Peter, c't 1/1990). */
static uint32_t tse_isqrt(uint32_t value)
{
    uint32_t result = 0UL;
    uint32_t bit = 1UL << 30UL;

    do
    {
        uint32_t trial = result + bit;

        result >>= 1U;
        if (trial <= value)
        {
            value -= trial;
            result += bit;
        }
        bit >>= 2U;
    } while (bit != 0UL);

    if (result < value)
    {
        result += 1UL;
    }

    return result;
}

void tse_init(void)
{
    SCU_GENERAL->PASSWD = SCU_PASSWD_UNLOCK;
    SCU_ANALOG->ANATSECTRL |= SCU_ANALOG_ANATSECTRL_TSE_EN_Msk;
    SCU_GENERAL->PASSWD = SCU_PASSWD_LOCK;

    /* The first conversion after enabling takes a few hundred microseconds. */
    while ((SCU_INTERRUPT->SRRAW & SCU_INTERRUPT_SRRAW_TSE_DONE_Msk) == 0UL)
    {
        /* wait for SRRAW.TSE_DONE */
    }
}

uint16_t tse_read_raw(void)
{
    return (uint16_t)SCU_ANALOG->ANATSEMON;
}

static uint32_t tse_calc_kelvin(uint32_t rx)
{
    int32_t k1 = (int32_t)*TSE_OTP_K1;
    int32_t k3 = (int32_t)*TSE_OTP_K3;
    int32_t k2 = (int32_t)*TSE_OTP_K2;
    int32_t h1;
    int32_t h3;

    /*
     * TSE_MON falls as the die gets hotter, so the "min" OTP entry clamps the
     * warm end and the "max" entry clamps the cold end. Naming follows the
     * Infineon reference implementation.
     */
    if (rx > (uint32_t)*TSE_OTP_MON_HI)
    {
        rx = (uint32_t)*TSE_OTP_MON_HI;
    }
    if (rx < (uint32_t)*TSE_OTP_MON_LO)
    {
        rx = (uint32_t)*TSE_OTP_MON_LO;
    }

    h1 = (int32_t)(rx * (uint32_t)k3);
    h1 -= k2;

    h3 = k1 * k1;
    h3 += h1;

    h3 = (int32_t)tse_isqrt((uint32_t)h3);
    h3 -= k1;
    h3 <<= 16;
    h1 >>= 7;
    h3 /= h1;

    return (uint32_t)((h3 >> 1) + (h3 & 1));
}

int32_t tse_read_millidegree_c(void)
{
    uint32_t kelvin;
    int32_t correction;

    kelvin = tse_calc_kelvin(tse_read_raw());

    if (kelvin < TSE_MIN_KELVIN)
    {
        kelvin = TSE_MIN_KELVIN;
    }
    else if (kelvin > TSE_MAX_KELVIN)
    {
        kelvin = TSE_MAX_KELVIN;
    }

    correction = (int32_t)TSE_OTP_CORR[kelvin - TSE_MIN_KELVIN];
    kelvin = (uint32_t)((int32_t)kelvin + correction);

    return ((int32_t)kelvin - (int32_t)TSE_KELVIN_OFFSET) * 1000;
}
