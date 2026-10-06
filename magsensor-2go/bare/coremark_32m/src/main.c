#include <stdint.h>
#include <stdio.h>

#include "board.h"
#include "system_xmc1100.h"
#include "utils.h"
#include "custom_def.h"
#include "core_portme.h"

int coremark_main(void);

int main(void)
{
    board_init();

    const uint32_t cpu_hz = board_cpu_frequency_hz();

    while (1)
    {
        printf("\r\n--- CoreMark run on XMC1100-Q024x0064 @ %lu Hz ---\r\n",
               (unsigned long)cpu_hz);
        coremark_main();
        printf("--- CoreMark complete. %lu Hz, %s ---\r\n",
               (unsigned long)cpu_hz, COMPILER_NAME);
        board_delay_ms(10000);
    }

    return 0;
}