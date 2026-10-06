#include <stdio.h>
#include "board.h"
#include "system_XMC4500.h"
#include "custom_def.h"
#include "dhry.h"

int main(void)
{
    board_init();

    const uint32_t cpu_hz = board_cpu_frequency_hz();

    printf("\r\n=== Dhrystone 2.1 on XMC4500-F100x1024 @ %lu Hz ===\r\n",
           (unsigned long)cpu_hz);

    while (1)
    {
        dhry_main(cpu_hz);
        printf("\r\nCPU freq: %lu Hz (%lu MHz)\r\n",
               (unsigned long)cpu_hz, (unsigned long)(cpu_hz / 1000000UL));
        printf("Compiler: %s\r\n", COMPILER_NAME);
        board_delay_ms(10000);
    }

    return 0;
}
