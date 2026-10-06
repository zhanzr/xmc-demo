/**
 * @file    armclang_compat.c
 * @brief   Thin printf wrapper for the Keil armclang toolchain.
 *
 * When building with armclang, cmake/armclang-keil-toolchain.cmake force-includes
 * cmake/printf_rename.h, which turns every `printf` call into `bench_printf`.
 * That stops armclang from emitting the ARMCLIB `__2printf` / `_printf_*` ABI,
 * which GNU ld + newlib cannot resolve. This file provides the renamed symbol;
 * it compiles to nothing for the GNU toolchain.
 */

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)

#include <stdarg.h>
#include <stdio.h>

int bench_printf(const char *fmt, ...)
{
    int result;
    va_list args;

    va_start(args, fmt);
    result = vprintf(fmt, args);
    va_end(args);

    return result;
}

#endif
