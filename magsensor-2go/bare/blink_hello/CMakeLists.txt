# LED blink + die temperature (DTS) demo for the "magsensor-2go" board
# (XMC1100-Q024x0064).
#
# Brings up the board layer, prints the CPU (MCLK) and peripheral (PCLK)
# frequencies over the USIC0 channel 0 console (TXD = P2.1, RXD = P2.2,
# J-Link-Lite VCP, 115200 8N1), then blinks both LEDs and periodically reports the
# die temperature sensor (DTS) reading with the current CPU clock.
#
# Toolchains:
#   bash build.sh                                        # gcc (default)
#   cmake -G Ninja -DXMC_TOOLCHAIN=armclang ..           # Keil AC6
#
# Pico-style workflow:
#   mkdir -p build && cd build
#   cmake -G Ninja ..
#   ninja           # builds .elf / .hex / .bin
#   ninja flash     # programs the board through J-Link (SWD)

cmake_minimum_required(VERSION 3.13)

# Toolchain selection. gcc (default) = GNU arm-none-eabi end-to-end;
# armclang = Keil Arm Compiler 6 C + GNU as/ld. CMAKE_TOOLCHAIN_FILE is cached,
# so use a separate build dir per toolchain (build/ for gcc, build-ac6/ for
# armclang).
set(XMC_TOOLCHAIN "gcc" CACHE STRING "C compiler: gcc (arm-none-eabi, default) or armclang (Keil AC6)")
set_property(CACHE XMC_TOOLCHAIN PROPERTY STRINGS gcc armclang)

if(NOT CMAKE_TOOLCHAIN_FILE)
    if(XMC_TOOLCHAIN STREQUAL "gcc")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/arm-none-eabi-toolchain.cmake")
    elseif(XMC_TOOLCHAIN STREQUAL "armclang")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/armclang-keil-toolchain.cmake")
    else()
        message(FATAL_ERROR "Unknown XMC_TOOLCHAIN '${XMC_TOOLCHAIN}' (expected gcc or armclang)")
    endif()
endif()

# When building with armclang, CMake's ARMClang module would otherwise inject
# its own --cpu/--march flags. We pass -mcpu ourselves, so opt out (see
# cmake/armclang-keil-toolchain.cmake for the matching toolchain-side setup).
if(POLICY CMP0123)
    cmake_policy(SET CMP0123 NEW)
endif()

project(blink_hello C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}.elf
    src/main.c
)

# The ARMClang CMake module forces a ".elf" executable suffix; the target name
# already carries it, so keep the output file name identical across toolchains.
set_target_properties(${PROJECT_NAME}.elf PROPERTIES SUFFIX "")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/xmc1100_board.cmake)
xmc1100_apply_board(${PROJECT_NAME}.elf "-O1")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/flash-targets.cmake)
