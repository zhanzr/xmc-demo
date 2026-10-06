# ST7735S 0.96" 160x80 LCD (md096) for the "magsensor-2go" board
# (XMC1100-Q024x0064). Two busses: bit-banged GPIO and USIC0_CH1 SPI.
#   SCL=P0.8 MOSI=P0.7 CS=P0.9 DC=P0.5 RES=P0.6 BL=P2.0, MISO=P2.6 unconnected.
# Built at -Os and with a single 6x12 font so the image fits the ~60 KB app
# region (see drivers/xmc1100x0064.ld).
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

project(st7735_md096_160x80 C ASM)

# Restore the GNU-style link rule; CMake's ARMClang module replaces it with an
# armlink rule that GNU ld cannot use.
if(XMC_TOOLCHAIN STREQUAL "armclang")
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/armclang-postproject.cmake)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME}.elf
    src/main.c
    src/interface.c
    src/lcd.c
    src/lcd/lcd_fonts.c
    src/backlight.c
)

# The ARMClang CMake module forces a ".elf" executable suffix; the target name
# already carries it, so keep the output file name identical across toolchains.
set_target_properties(${PROJECT_NAME}.elf PROPERTIES SUFFIX "")

# Static arena for newlib stdio, in case a stream still wants a buffer.
set(XMC_HEAP_SIZE 4096)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/xmc1100_board.cmake)
xmc1100_apply_board(${PROJECT_NAME}.elf "-Os")

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    src
    src/blockwrite
    src/lcd
)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/flash-targets.cmake)