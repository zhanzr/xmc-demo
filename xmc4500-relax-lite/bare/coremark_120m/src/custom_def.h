#ifndef __CUSTOM_DEF_H__
#define __CUSTOM_DEF_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#ifndef configTICK_RATE_HZ
#define	configTICK_RATE_HZ	1000
#endif

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#if defined(__ARMCC_VERSION)
    #define COMPILER_NAME "Arm Compiler " TOSTRING(__ARMCC_VERSION)
#elif defined(__GNUC__)
    #define COMPILER_NAME "GCC " __VERSION__
#else
    #define COMPILER_NAME "Unknown Compiler"
#endif

#endif