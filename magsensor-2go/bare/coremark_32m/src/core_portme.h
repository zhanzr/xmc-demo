#ifndef CORE_PORTME_H
#define CORE_PORTME_H

#if defined(__clang__)
#   pragma clang diagnostic ignored "-Wunknown-warning-option"
#   pragma clang diagnostic ignored "-Wreserved-identifier"
#   pragma clang diagnostic ignored "-Wundef"
#   pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#   pragma clang diagnostic ignored "-Wpadded"
#   pragma clang diagnostic ignored "-Wstrict-prototypes"
#   pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#ifndef HAS_FLOAT
#define HAS_FLOAT 1
#endif
#ifndef HAS_TIME_H
#define HAS_TIME_H 0
#endif
#ifndef USE_CLOCK
#define USE_CLOCK 0
#endif
#ifndef HAS_STDIO
#define HAS_STDIO 1
#endif
#ifndef HAS_PRINTF
#define HAS_PRINTF 0
#endif

/* Console output: newlib printf, retargeted to the USIC0 ch0 console in
 * board/syscalls.c (_write -> console_write). Works with gcc and, through the
 * printf -> bench_printf rename in cmake/printf_rename.h, with armclang too. */
#define ee_printf printf

#include <stdint.h>
#include <stddef.h>

#ifndef STRINGIFY
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#endif

/* Report the flags actually used: AC6 rejects -funroll-all-loops and gets
   -funroll-loops instead (see the BENCH_OPT default in CMakeLists.txt). */
#if defined(__ARMCC_VERSION)
#define FLAGS_STR "-Ofast -ffp-contract=fast -funroll-loops"
#else
#define FLAGS_STR "-Ofast -ffp-contract=fast -funroll-all-loops"
#endif
/* Three phases of ITERATIONS each, and results under 10 s are rejected.
   500 iterations ~ 11.8 s at the measured 42 iterations/s. */
#ifndef ITERATIONS
#define ITERATIONS 500
#endif

typedef uint32_t CORE_TICKS;

#ifndef COMPILER_VERSION
#if defined(__ARMCC_VERSION)
#define COMPILER_VERSION "Arm Compiler " TOSTRING(__ARMCC_VERSION)
#elif defined(__clang__)
#define COMPILER_VERSION "Clang " __VERSION__
#elif defined(__GNUC__)
#define COMPILER_VERSION "GCC " __VERSION__
#else
#define COMPILER_VERSION "Unknown"
#endif
#endif
#ifndef COMPILER_FLAGS
#define COMPILER_FLAGS FLAGS_STR
#endif

#ifndef MEM_LOCATION
#define MEM_LOCATION "Static"
#endif

#ifndef MEM_METHOD
#define MEM_METHOD MEM_STATIC
#endif

typedef signed short ee_s16;
typedef unsigned short ee_u16;
typedef signed int ee_s32;
typedef double ee_f32;
typedef unsigned char ee_u8;
typedef unsigned int ee_u32;
typedef ee_u32 ee_ptr_int;
typedef size_t ee_size_t;

#define align_mem(x) (void *)(4 + (((ee_ptr_int)(x) - 1) & ~3))

#ifndef SEED_METHOD
#define SEED_METHOD SEED_VOLATILE
#endif

#ifndef MULTITHREAD
#define MULTITHREAD 1
#define USE_PTHREAD 0
#define USE_FORK 0
#define USE_SOCKET 0
#endif

#ifndef MAIN_HAS_NOARGC
#define MAIN_HAS_NOARGC 1
#endif

#ifndef MAIN_HAS_NORETURN
#define MAIN_HAS_NORETURN 0
#endif

extern ee_u32 default_num_contexts;

typedef struct CORE_PORTABLE_S {
	ee_u8 portable_id;
} core_portable;

int coremark_main(void);

void portable_init(core_portable *p, int *argc, char *argv[]);
void portable_fini(core_portable *p);

#if !defined(PROFILE_RUN) && !defined(PERFORMANCE_RUN) && !defined(VALIDATION_RUN)
#if (TOTAL_DATA_SIZE==1200)
#define PROFILE_RUN 1
#elif (TOTAL_DATA_SIZE==2000)
#define PERFORMANCE_RUN 1
#else
#define VALIDATION_RUN 1
#endif
#endif

#endif