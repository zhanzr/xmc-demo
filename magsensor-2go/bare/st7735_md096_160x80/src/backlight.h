/**
 * @file    backlight.h
 * @brief   ST7735 backlight (BL = P2.0), CCU40 slice-0 PWM (~100 kHz).
 */

#ifndef __BACKLIGHT_H
#define __BACKLIGHT_H

#include <stdint.h>

void Backlight_Init(void);
void Backlight_SetDuty(uint16_t percent);
uint16_t Backlight_GetDuty(void);      /* current duty in percent */

#endif /* __BACKLIGHT_H */