/**
  * @file    armclang_force_wint_t.h
  * @brief   Force-included when compiling with Keil armclang + GNU newlib.
  *
  * armclang bundles its own ARMCLIB <stddef.h> (in ARMCLANG/include), which
  * shadows the LLVM stddef.h and does NOT implement the newlib "set __need_wint_t
  * then include <stddef.h>" protocol. As a result newlib's sys/_types.h cannot
  * obtain wint_t. This shim defines it up front (Keil's stddef.h never defines
  * wint_t, and LLVM's __stddef_wint_t.h uses the same _WINT_T guard).
  */

#ifndef __ARMCLANG_FORCE_WINT_T_H
#define __ARMCLANG_FORCE_WINT_T_H

#if !defined(_WINT_T)
#define _WINT_T
typedef int wint_t;
#endif

#endif /* __ARMCLANG_FORCE_WINT_T_H */
