# CoreMark 1.0.1 benchmark for the XMC1100-Q024x0064 "magsensor-2go" board @ 32 MHz.
# Default compiler is GNU arm-none-eabi-gcc at the most aggressive optimization
# (-Ofast -ffp-contract=fast -funroll-all-loops). Keil Arm Compiler 6 (armclang)
# is available with -DXMC_TOOLCHAIN=armclang; the unroll flag then defaults to
# the -funroll-loops that AC6 accepts.
#
# Pico-style workflow:
#   bash build.sh                                               # gcc (default)
#   cmake -G Ninja -S . -B build-ac6 -DXMC_TOOLCHAIN=armclang   # Keil AC6
#   cmake -G Ninja -S . -B build-lto -DXMC_LTO=ON              # gcc + LTO
#   ninja           # builds .elf / .hex / .bin
#   ninja flash     # programs the board (J-Link, SWD)

cmake_minimum_required(VERSION 3.13)

# We pass the cpu/arch flags ourselves (-mcpu=...) and link with GNU ld, so
# keep CMake's ARMClang module from auto-adding --cpu/--march flags.
if(POLICY CMP0123)
    cmake_policy(SET CMP0123 NEW)
endif()

# Toolchain selection. gcc (default) = GNU arm-none-eabi end-to-end;
# armclang = Keil Arm Compiler 6 C + GNU as/ld. CMAKE_TOOLCHAIN_FILE is cached,
# so use a separate build dir per toolchain (build/ for gcc, build-ac6/ for
# armclang).
set(XMC_TOOLCHAIN "gcc" CACHE STRING "C compiler: gcc (arm-none-eabi, default) or armclang (Keil AC6)")
set_property(CACHE XMC_TOOLCHAIN PROPERTY STRINGS gcc armclang)

# Link-time optimization. GCC only: armclang -flto/-Omax emits LLVM bitcode
# (.llvm.lto) that needs armlink + ARMCLIB and cannot be consumed by GNU ld.
option(XMC_LTO "Enable -flto with the GCC toolchain" OFF)

if(NOT CMAKE_TOOLCHAIN_FILE)
    if(XMC_TOOLCHAIN STREQUAL "gcc")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/arm-none-eabi-toolchain.cmake")
    elseif(XMC_TOOLCHAIN STREQUAL "armclang")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/armclang-keil-toolchain.cmake")
    else()
        message(FATAL_ERROR "Unknown XMC_TOOLCHAIN '${XMC_TOOLCHAIN}' (expected gcc or armclang)")
    endif()
endif()

project(coremark_32m C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# -funroll-all-loops is GCC-only; Arm Compiler 6 rejects it with
# -Wignored-optimization-argument, so use the -funroll-loops it does accept.
if(XMC_TOOLCHAIN STREQUAL "armclang")
    set(BENCH_OPT_DEFAULT "-Ofast -ffp-contract=fast -funroll-loops")
else()
    set(BENCH_OPT_DEFAULT "-Ofast -ffp-contract=fast -funroll-all-loops")
endif()
set(BENCH_OPT "${BENCH_OPT_DEFAULT}" CACHE STRING
    "Optimization flags for xmc1100_apply_board (e.g. -Omax for armclang)")

# C-language-only optimization flags, applied to the C sources but NOT to the
# GNU-as startup/toolchain files or the link step. Required for armclang-only
# options that arm-none-eabi-gcc rejects (e.g. '-Omax').
set(BENCH_OPT_C "" CACHE STRING
    "Extra C-language-only optimization flags (e.g. armclang '-Omax'; not passed to ASM/link)")
set(CORE ${CMAKE_CURRENT_SOURCE_DIR}/coremark_1_0_1)

add_executable(${PROJECT_NAME}.elf
    src/main.c
    src/utils.c
    src/core_portme.c
    ${CORE}/core_list_join.c
    ${CORE}/core_main.c
    ${CORE}/core_matrix.c
    ${CORE}/core_state.c
    ${CORE}/core_util.c
)

# CoreMark itself runs with MEM_STATIC (no malloc), but newlib's stdio still
# allocates a BUFSIZ buffer per stream on first use. Without an arena printf()
# trips a "Balloc succeeded" assertion and the run stops at the first report.
# Must be set before including the board module, which consumes it.
set(XMC_HEAP_SIZE 4096)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/xmc1100_board.cmake)
xmc1100_apply_board(${PROJECT_NAME}.elf "${BENCH_OPT}")

if(XMC_LTO)
    target_compile_options(${PROJECT_NAME}.elf PRIVATE
        "$<$<COMPILE_LANGUAGE:C>:-flto>"
        -ffat-lto-objects)
    target_link_options(${PROJECT_NAME}.elf PRIVATE -flto)
endif()

if(BENCH_OPT_C)
    target_compile_options(${PROJECT_NAME}.elf PRIVATE
        "$<$<COMPILE_LANGUAGE:C>:${BENCH_OPT_C}>")
endif()

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    src
    ${CORE}
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/flash-targets.cmake)