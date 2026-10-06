#include "board.h"
#include "utils.h"

void TICK_Init(void)
{
    /* board_init() already configured the 1 kHz SysTick used for timing. */
}

uint32_t HAL_GetTick(void)
{
    return board_millis();
}

void HAL_Delay(uint32_t t)
{
    board_delay_ms(t);
}