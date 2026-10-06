/**
 * @file    backlight.c
 * @brief   ST7735 backlight (BL = P2.0), CCU40 slice-0 PWM.
 *
 * P2.0 = CCU40_OUT0 (AF2); it is a PORT2 pad, so its PDISC bit must be
 * cleared. Slice clock = fPCLK (64 MHz), period 640 counts -> ~100 kHz PWM.
 * Compare shadow: compare = period * (100 - %duty) / 100, latched on the
 * next period match via the S0SE shadow transfer.
 */

#include "backlight.h"

#include "lcd.h"
#include "interface.h"

#include "XMC1100.h"

#define BL_PERIOD   (640U)          /* ticks per cycle (~100 kHz) */
#define BL_AF2      (0x90UL)        /* P2.0 -> CCU40 slice-0 OUT (AF2) */

static uint16_t s_duty = 100U;

static uint32_t bl_duty_compare(uint16_t percent)
{
    uint32_t compare;

    if (percent > 100U)
    {
        percent = 100U;
    }

    /* CCU4 compare mode here: output is low while the counter < CR and high
     * from CR to the period, so high-time = period - CR. CR=0 is degenerate
     * (the output stays low), so keep CR within [1, period-1]. */
    compare = (uint32_t)(BL_PERIOD * (100U - percent)) / 100U;
    if (compare < 1U)
    {
        compare = 1U;
    }
    if (compare > (BL_PERIOD - 1U))
    {
        compare = BL_PERIOD - 1U;
    }

    return compare;
}

static void bl_set_compare(uint32_t compare)
{
    CCU40_CC40->CRS = compare;         /* shadow register */
    CCU40->GCSS = CCU4_GCSS_S0SE_Msk;  /* latch PR/CR/PSL at period match */
}

void Backlight_Init(void)
{
    lcd_io_bind_af(LCD_BL_PORT, LCD_BL_PIN, BL_AF2);  /* clears P2.0 PDISC */

    /* Ungate the CCU40 peripheral clock in the SCU (PASSWD-protected). Without
     * this the slice counter never runs and the output stays at the passive
     * level, i.e. the backlight never lights. */
    SCU_GENERAL->PASSWD = 0x000000C0UL;
    SCU_CLK->CGATCLR0 = SCU_CLK_CGATSTAT0_CCU40_Msk;
    SCU_GENERAL->PASSWD = 0x000000C3UL;
    while ((SCU_CLK->CGATSTAT0 & SCU_CLK_CGATSTAT0_CCU40_Msk) != 0U)
    {
        /* wait until the CCU40 clock is running */
    }

    CCU40->GIDLC = CCU4_GIDLC_SPRB_Msk;    /* module prescaler running */
    CCU40->GIDLC |= 1UL;                   /* slice-0 clock on */

    /* edge-aligned up-counter, repeat, prescaler normal (all-zero TC) */
    CCU40_CC40->TC   = 0U;
    CCU40_CC40->PSC  = 0U;
    CCU40_CC40->DITS = 0U;
    CCU40_CC40->PSL  = 0U;                 /* passive level low */
    CCU40_CC40->CMC  = 0U;

    CCU40_CC40->PRS = BL_PERIOD - 1U;
    bl_set_compare(bl_duty_compare(100U));

    CCU40_CC40->TCSET = CCU4_CC4_TCSET_TRBS_Msk;   /* run */
    s_duty = 100U;
}

void Backlight_SetDuty(uint16_t percent)
{
    bl_set_compare(bl_duty_compare(percent));
    s_duty = percent;
}

uint16_t Backlight_GetDuty(void)
{
    return s_duty;
}