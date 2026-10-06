/**
 * @file    tse.h
 * @brief   XMC1100 die temperature sensor (TSE).
 *
 * The XMC1100 VADC has no internal channels at all (reference manual Table 15-4
 * lists external P2.x pads only), so the on-die temperature sensor is the only
 * internal analog measurement available. It is read through
 * SCU_ANALOG->ANATSEMON and calibrated with the OTP data in flash sector 0.
 */
#ifndef TSE_H
#define TSE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Temperature range covered by the OTP correction table, in Kelvin (-40 C .. 125 C). */
#define TSE_MIN_KELVIN (233U)
#define TSE_MAX_KELVIN (398U)

/** Enable the die temperature sensor and wait for the first conversion to finish. */
void tse_init(void);

/**
 * @brief Read the die temperature.
 *
 * @return temperature in millidegrees Celsius (e.g. 25345 == 25.345 C), already
 *         clamped to the calibrated -40 C .. 125 C range.
 */
int32_t tse_read_millidegree_c(void);

/** @brief Raw, uncorrected TSE_MON counter value (for diagnostics). */
uint16_t tse_read_raw(void);

#ifdef __cplusplus
}
#endif

#endif /* TSE_H */
