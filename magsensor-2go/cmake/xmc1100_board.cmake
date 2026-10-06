# Shared board layer for the XMC1100-Q024x0064 "magsensor-2go" board projects.
#
# Attaches the board support (clock tree, SysTick tick, LEDs, USIC0 channel 0
# console, die temperature sensor, newlib stubs), the DFP startup file and the
# linker script to a target with xmc1100_apply_board().
#
# Usage (from a project CMakeLists.txt, after add_executable()):
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/xmc1100_board.cmake)
#   xmc1100_apply_board(${PROJECT_NAME}.elf "-O1")
#
# Requires the project to enable ASM (project(X C ASM)).
#
# External inputs (all overridable from the command line):
#   -DDRIVERS_ROOT=...   repo-root drivers/ container (one vendored folder per XMC series)
#   -DXMC_SERIES=...     which drivers/<series>/ folder this board builds against
#   -DXMC_DFP_ROOT=...   full Infineon XMC1000_DFP, used instead of the vendored one
#   -DCMSIS_ROOT=...     full ARM CMSIS pack, used instead of the vendored one

set(BOARD_DIR ${CMAKE_CURRENT_LIST_DIR}/../board)

set(DRIVERS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../drivers" CACHE PATH
    "Repo-root drivers/ container with one vendored folder per XMC series")
set(XMC_SERIES "xmc1" CACHE STRING
    "Which drivers/<series>/ vendored folder this board builds against (xmc1, xmc4, ...)")
set(XMC_DFP_ROOT "" CACHE PATH
    "Root of a full Infineon XMC1000 Device Family Pack (optional; overrides vendored device headers)")
set(CMSIS_ROOT "" CACHE PATH
    "Root of a full ARM CMSIS pack (optional; overrides vendored core headers)")

if(XMC_DFP_ROOT)
    set(XMC_DEVICE_INC ${XMC_DFP_ROOT}/Device/XMC1100_series/Include)
else()
    set(XMC_DEVICE_INC ${DRIVERS_ROOT}/${XMC_SERIES}/CMSIS/Include/Device/Infineon/XMC1100_series/Include)
endif()

if(CMSIS_ROOT)
    set(CMSIS_CORE_INC ${CMSIS_ROOT}/CMSIS/Core/Include)
else()
    set(CMSIS_CORE_INC ${DRIVERS_ROOT}/${XMC_SERIES}/CMSIS/Include)
endif()

# The DFP linker script defaults to a 1 KB stack, which newlib's printf blows
# through (_vfprintf_r + __ssprint_r + _write nest ~1.7 KB deep on ARM EABI
# Thumb, and Cortex-M0 has no bus fault, so the overflow silently walks the stack
# pointer off the end of SRAM into unmapped space and the next exception entry
# loses its frame). There is no heap in this script, so the whole region between
# the veneers and .data is available to the stack.
set(BOARD_STACK_SIZE "3072" CACHE STRING "Stack size in bytes for the linker script")

# Linker script. Default is the board's standard one; a project may point this at
# its own script by setting BOARD_LINKER_SCRIPT before including this file.
if(NOT BOARD_LINKER_SCRIPT)
    set(BOARD_LINKER_SCRIPT ${BOARD_DIR}/xmc1100x0064.ld)
endif()

function(_xmc1100_require_file FILE_VAR)
    if(NOT EXISTS "${${FILE_VAR}}")
        message(FATAL_ERROR
            "${${FILE_VAR}} not found. Pass -D${FILE_VAR}=/path/to/... on the cmake command line.")
    endif()
endfunction()

function(xmc1100_apply_board TGT OPT)
    separate_arguments(OPT_LIST NATIVE_COMMAND "${OPT}")

    _xmc1100_require_file(XMC_DEVICE_INC)
    _xmc1100_require_file(CMSIS_CORE_INC)
    _xmc1100_require_file(BOARD_LINKER_SCRIPT)

    # GCC-only warning switches; keep clang-based toolchains clean.
    if(XMC_ARMCLANG)
        set(_WARN_FLAGS -Wall -Wno-unused-command-line-argument)
    else()
        set(_WARN_FLAGS
            -Wall
            -Wno-unused-but-set-variable -Wno-unused-function
            -Wno-unused-variable -Wno-unused-parameter -Wno-maybe-uninitialized)
    endif()

    target_sources(${TGT} PRIVATE
        ${BOARD_DIR}/board.c
        ${BOARD_DIR}/console.c
        ${BOARD_DIR}/system_xmc1100.c
        ${BOARD_DIR}/tse.c
        ${BOARD_DIR}/syscalls.c
        ${BOARD_DIR}/armclang_compat.c
        ${BOARD_DIR}/startup_XMC1100.S
    )

    # Heap, off by default. Set XMC_HEAP_SIZE before including this module to serve
    # _sbrk from a static arena: needed by anything that allocates, including
    # newlib's stdio (a buffer per stream on first use).
    if(NOT DEFINED XMC_HEAP_SIZE)
        set(XMC_HEAP_SIZE 0)
    endif()
    if(XMC_HEAP_SIZE GREATER 0)
        set_source_files_properties(${BOARD_DIR}/syscalls.c PROPERTIES
            COMPILE_DEFINITIONS XMC_HEAP_SIZE=${XMC_HEAP_SIZE})
    endif()

    target_include_directories(${TGT} PRIVATE
        ${BOARD_DIR}
        ${XMC_DEVICE_INC}
        ${CMSIS_CORE_INC}
    )

    # armclang has no bundled libc headers: point it at the GNU newlib include
    # dir so <stdio.h>/<string.h>/... resolve to the same newlib we link.
    if(XMC_ARMCLANG)
        target_include_directories(${TGT} SYSTEM PRIVATE
            "${ARM_GCC_ROOT}/arm-none-eabi/include"
        )
    endif()

    target_compile_options(${TGT} PRIVATE
        -mcpu=cortex-m0 -mthumb
        ${OPT_LIST} -g
        -ffunction-sections -fdata-sections ${_WARN_FLAGS}
    )

    set(_LDFLAGS "-mcpu=cortex-m0 -mthumb ${OPT}")
    set(_LDFLAGS "${_LDFLAGS} -Wl,--gc-sections -nostartfiles")
    set(_LDFLAGS "${_LDFLAGS} -Wl,-Map=${PROJECT_NAME}.map")
    set(_LDFLAGS "${_LDFLAGS} -T ${BOARD_LINKER_SCRIPT}")
    set(_LDFLAGS "${_LDFLAGS} -Wl,--defsym=stack_size=${BOARD_STACK_SIZE}")
    set(_LDFLAGS "${_LDFLAGS} -lc -lm")

    # newlib/libgcc's thumb/v6-m/nofp multilib objects are built with
    # -fshort-enums and lack .note.GNU-stack, so a GNU ld link spews a handful
    # of benign warnings. The sizes match the ARM EABI defaults our objects use,
    # so this is noise, not an ABI error.
    set(_LDFLAGS "${_LDFLAGS} -Wl,--no-enum-size-warning -Wl,--no-wchar-size-warning -Wl,--no-warn-execstack")

    set_target_properties(${TGT} PROPERTIES
        LINK_FLAGS "${_LDFLAGS}"
    )

    # CMake's ARMClang module hardcodes a bare trailing "-Xlinker" into its link
    # rule (it assumes armlink, which needs every linker arg routed through
    # -Xlinker). We drive GNU ld through arm-none-eabi-gcc instead, which rejects
    # a dangling -Xlinker, so restore a plain rule. Must be PARENT_SCOPE: the
    # rule is generated in the calling directory.
    if(XMC_ARMCLANG)
        set(CMAKE_C_LINK_EXECUTABLE
            "<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>"
            PARENT_SCOPE)
    endif()
endfunction()
