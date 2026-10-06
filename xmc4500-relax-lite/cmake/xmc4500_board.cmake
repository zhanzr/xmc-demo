# Shared board layer for the XMC4500-F100x1024 "xmc4500-relax-lite" board projects.
#
# Attaches the board support (clock tree from the DFP startup, SysTick tick, LEDs,
# USIC0 channel 0 console, newlib stubs), the DFP startup file, the linker script
# and the small slice of XMClib the board needs (SCU die-temperature sensor and
# EVR13/EVR33 monitors) to a target with xmc4500_apply_board().
#
# Usage (from a project CMakeLists.txt, after add_executable()):
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/xmc4500_board.cmake)
#   xmc4500_apply_board(${PROJECT_NAME}.elf "-O1")
#
# Requires the project to enable ASM (project(X C ASM)).
#
# External inputs (all overridable from the command line):
#   -DDRIVERS_ROOT=...   repo-root drivers/ container (one vendored folder per XMC series)
#   -DXMC_SERIES=...     which drivers/<series>/ folder this board builds against
#   -DXMC_DFP_ROOT=...   full Infineon XMC4000_DFP, used instead of the vendored one
#   -DCMSIS_ROOT=...     full ARM CMSIS pack, used instead of the vendored one

set(BOARD_DIR ${CMAKE_CURRENT_LIST_DIR}/../board)

set(DRIVERS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../drivers" CACHE PATH
    "Repo-root drivers/ container with one vendored folder per XMC series")
set(XMC_SERIES "xmc4" CACHE STRING
    "Which drivers/<series>/ vendored folder this board builds against (xmc1, xmc4, ...)")
set(XMC_DFP_ROOT "" CACHE PATH
    "Root of a full Infineon XMC4000 Device Family Pack (optional; overrides vendored device headers)")
set(CMSIS_ROOT "" CACHE PATH
    "Root of a full ARM CMSIS pack (optional; overrides vendored core headers)")

if(XMC_DFP_ROOT)
    set(XMC_DEVICE_INC ${XMC_DFP_ROOT}/Device/XMC4500_series/Include)
else()
    set(XMC_DEVICE_INC ${DRIVERS_ROOT}/${XMC_SERIES}/CMSIS/Include/Device/Infineon/XMC4500_series/Include)
endif()

if(CMSIS_ROOT)
    set(CMSIS_CORE_INC ${CMSIS_ROOT}/CMSIS/Core/Include)
else()
    set(CMSIS_CORE_INC ${DRIVERS_ROOT}/${XMC_SERIES}/CMSIS/Include)
endif()

set(XMCLIB_ROOT ${DRIVERS_ROOT}/${XMC_SERIES}/XMClib)
set(XMCLIB_INC ${XMCLIB_ROOT}/inc)
set(XMCLIB_SRC ${XMCLIB_ROOT}/src)

# Only the SCU driver is needed (die temperature + EVR13/EVR33). Compiled with
# -ffunction-sections/--gc-sections so projects that do not use it drop it.
set(BOARD_XMCLIB_SOURCES ${XMCLIB_SRC}/xmc4_scu.c)

# The DFP linker script defaults to a 2 KB stack, which newlib's printf blows
# through (_vfprintf_r + __ssprint_r + _write nest ~1.7 KB deep on ARM EABI
# Thumb). The stack lives in PSRAM_1, so there is plenty of room.
set(BOARD_STACK_SIZE "8192" CACHE STRING "Stack size in bytes for the linker script")

# Linker script. Default is the board's standard one; a project may point this at
# its own script by setting BOARD_LINKER_SCRIPT before including this file.
if(NOT BOARD_LINKER_SCRIPT)
    set(BOARD_LINKER_SCRIPT ${BOARD_DIR}/XMC4500x1024.ld)
endif()

function(_xmc4500_require_file FILE_VAR)
    if(NOT EXISTS "${${FILE_VAR}}")
        message(FATAL_ERROR
            "${${FILE_VAR}} not found. Pass -D${FILE_VAR}=/path/to/... on the cmake command line.")
    endif()
endfunction()

function(xmc4500_apply_board TGT OPT)
    separate_arguments(OPT_LIST NATIVE_COMMAND "${OPT}")

    _xmc4500_require_file(XMC_DEVICE_INC)
    _xmc4500_require_file(CMSIS_CORE_INC)
    _xmc4500_require_file(XMCLIB_INC)
    _xmc4500_require_file(BOARD_LINKER_SCRIPT)

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
        ${BOARD_DIR}/system_XMC4500.c
        ${BOARD_DIR}/syscalls.c
        ${BOARD_DIR}/armclang_compat.c
        ${BOARD_DIR}/startup_XMC4500.S
        ${BOARD_XMCLIB_SOURCES}
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
        ${XMCLIB_INC}
    )

    # armclang has no bundled libc headers: point it at the GNU newlib include
    # dir so <stdio.h>/<string.h>/... resolve to the same newlib we link.
    if(XMC_ARMCLANG)
        target_include_directories(${TGT} SYSTEM PRIVATE
            "${ARM_GCC_ROOT}/arm-none-eabi/include"
        )
    endif()

    # Part number for the vendored XMClib's xmc_device.h (selects UC_DEVICE /
    # UC_SERIES / UC_PACKAGE and the device header).
    target_compile_definitions(${TGT} PRIVATE XMC4500_F100x1024)

    target_compile_options(${TGT} PRIVATE
        -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
        ${OPT_LIST} -g
        -ffunction-sections -fdata-sections ${_WARN_FLAGS}
    )

    set(_CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
    set(_LDFLAGS "${_CPU_FLAGS} ${OPT}")
    set(_LDFLAGS "${_LDFLAGS} -Wl,--gc-sections -nostartfiles")
    set(_LDFLAGS "${_LDFLAGS} -Wl,-Map=${PROJECT_NAME}.map")
    set(_LDFLAGS "${_LDFLAGS} -T ${BOARD_LINKER_SCRIPT}")
    set(_LDFLAGS "${_LDFLAGS} -Wl,--defsym=stack_size=${BOARD_STACK_SIZE}")
    set(_LDFLAGS "${_LDFLAGS} -lc -lm")

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
