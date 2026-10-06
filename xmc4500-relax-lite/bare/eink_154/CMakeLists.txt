# 1.54" e-paper (SSD1608 / Waveshare epd1in54-class) bring-up for the
# "xmc4500-relax-lite" board (XMC4500-F100x1024).
#
# Drives the panel from USIC2 channel 0 hardware SPI (SCK = P5.2,
# MOSI = P5.0) plus GPIO CS/DC/RES/BUSY on P5.1 / P5.7 / P3.3 / P3.4 (all on the
# X2 header). Prints progress over the debug console (USIC1_CH0, P0.5/P0.4).
#
# Toolchains:
#   bash build.sh                                        # gcc (default)
#   cmake -G Ninja -DXMC_TOOLCHAIN=armclang ..           # Keil AC6

cmake_minimum_required(VERSION 3.13)

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

if(POLICY CMP0123)
    cmake_policy(SET CMP0123 NEW)
endif()

project(eink_154 C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}.elf
    src/main.c
    src/epdif.c
    src/epd1in54.c
    src/canvas.c
    src/font8.c
)

set_target_properties(${PROJECT_NAME}.elf PROPERTIES SUFFIX "")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/xmc4500_board.cmake)
xmc4500_apply_board(${PROJECT_NAME}.elf "-O1")

# XMClib sources for the panel path: USIC SPI master (hardware SPI), the USIC
# core (baud rate) and GPIO helpers.
target_sources(${PROJECT_NAME}.elf PRIVATE
    ${XMCLIB_SRC}/xmc_spi.c
    ${XMCLIB_SRC}/xmc_usic.c
    ${XMCLIB_SRC}/xmc_gpio.c
    ${XMCLIB_SRC}/xmc_common.c
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/flash-targets.cmake)
