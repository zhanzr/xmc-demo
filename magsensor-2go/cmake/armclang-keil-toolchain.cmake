# CMake toolchain file that uses Keil Arm Compiler 6 (armclang) for C
# compilation, while assembling and linking with the GNU arm-none-eabi tools.
#
# armclang is the LLVM/clang-based compiler shipped with Keil MDK. We still link
# with GNU ld so the existing linker script (xmc1100x0064.ld), newlib-based
# syscalls.c and the DFP startup file all keep working, and J-Link flashing stays
# identical.
#
# Selection (project CMakeLists):
#   cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=../../cmake/armclang-keil-toolchain.cmake ..
#
# Variables:
#   ARMCLANG_ROOT : Keil MDK ARMCLANG dir (contains bin/armclang.exe)
#   ARM_GCC_ROOT  : GNU arm-none-eabi root (assembler, linker, newlib, objcopy)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR cortex-m0)

# CMake's ARMClang module normally injects --cpu/--march and wraps the link line
# with -Xlinker. We pass the cpu/arch flags ourselves (-mcpu=...) and link with
# GNU ld, so opt out of that behaviour. CMAKE_POLICY_DEFAULT_* is needed because
# a plain cmake_policy() call inside a toolchain file only affects that file.
if(POLICY CMP0123)
    cmake_policy(SET CMP0123 NEW)
endif()
set(CMAKE_POLICY_DEFAULT_CMP0123 NEW)

set(ARMCLANG_ROOT "D:/Keil_v5/ARM/ARMCLANG" CACHE PATH
    "Keil Arm Compiler 6 root (contains bin/armclang.exe)")
set(ARM_GCC_ROOT "D:/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi" CACHE PATH
    "GNU arm-none-eabi root (assembler, linker, newlib)")

# Compile C with armclang (LLVM). The explicit target is required: armclang
# otherwise defaults to "unspecified-arm-none-none". -mcpu keeps even the
# configure-time sanity compiles on the right core. The -include shim fixes the
# missing newlib wint_t (armclang's bundled stddef.h shadows LLVM's and ignores
# the __need_wint_t protocol).
set(CMAKE_C_COMPILER "${ARMCLANG_ROOT}/bin/armclang.exe")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --target=arm-arm-none-eabi -mcpu=cortex-m0 -mthumb")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${CMAKE_CURRENT_LIST_DIR}/armclang_force_wint_t.h")
# Rename printf -> bench_printf so armclang does not emit the ARMCLIB
# __2printf/_printf_* ABI (which GNU ld + newlib cannot resolve). The wrapper
# lives in board/armclang_compat.c.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${CMAKE_CURRENT_LIST_DIR}/printf_rename.h")

# The DFP startup file uses GNU as syntax: assemble it with the GNU assembler.
set(CMAKE_ASM_COMPILER "${ARM_GCC_ROOT}/bin/arm-none-eabi-gcc.exe")

# Link with the GNU gcc driver (GNU ld + our .ld script + newlib). armclang
# objects are standard ELF and link cleanly.
set(CMAKE_LINKER "${ARM_GCC_ROOT}/bin/arm-none-eabi-gcc.exe")

# CMake's ARMClang module builds archives with `armar --create -cr`, so the
# archive tool must be Keil's armar (only used for configure-time checks).
set(CMAKE_AR "${ARMCLANG_ROOT}/bin/armar.exe")

# Object tools operate on the (ELF) output of the GNU link.
set(CMAKE_OBJCOPY "${ARM_GCC_ROOT}/bin/arm-none-eabi-objcopy.exe")
set(CMAKE_OBJDUMP "${ARM_GCC_ROOT}/bin/arm-none-eabi-objdump.exe")
set(CMAKE_SIZE "${ARM_GCC_ROOT}/bin/arm-none-eabi-size.exe")

# Bare-metal target: compiler sanity check links against a static library so
# no executable link step is needed during configure.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Route the executable link through the GNU driver instead of armclang.
set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_LINKER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Tells cmake/xmc1100_board.cmake to apply armclang-specific tweaks
# (newlib include path, clang-compatible warning flags).
set(XMC_ARMCLANG TRUE)
