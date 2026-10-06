#ifndef __UTILS_H__
#define	__UTILS_H__

#include <stdint.h>

#define RAM_FUNC

#define PRINTF(...)  printf(__VA_ARGS__)

void TICK_Init(void);

/* CoreMark's timing hooks: the board's 1 kHz millisecond tick. */
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t t);

#endif