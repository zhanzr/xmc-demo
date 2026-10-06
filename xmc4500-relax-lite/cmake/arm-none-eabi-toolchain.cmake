# CMake toolchain file for Arm bare-metal XMC4500 (Cortex-M4F) builds with the
# GNU Arm Embedded toolchain.
#
# Each project CMakeLists includes this automatically via CMAKE_TOOLCHAIN_FILE
# unless one is passed with -DCMAKE_TOOLCHAIN_FILE. Example flow:
#   mkdir -p build && cd build
#   cmake -G Ninja ..                                  # defaults below
#   cmake -G Ninja -DARM_GCC_ROOT=/path/to/arm-none-eabi ..
#
# ARM_GCC_ROOT: directory containing the toolchain's "bin" subfolder.
# - Windows: defaults to the toolchain used for this repo.
# - Linux/macOS: defaults to PATH lookup (arm-none-eabi-gcc must be installed).

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Bare-metal target: link the compiler check against a static library so no
# executable link step is needed during the initial compiler sanity test.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(CMAKE_HOST_WIN32)
    set(ARM_GCC_ROOT "D:/Arm/GNU Toolchain mingw-w64-x86_64-arm-none-eabi" CACHE PATH
        "Root dir of the arm-none-eabi toolchain (contains bin/)")
else()
    set(ARM_GCC_ROOT "" CACHE PATH
        "Root dir of the arm-none-eabi toolchain (contains bin/)")
endif()

if(ARM_GCC_ROOT)
    set(_TOOLBIN "${ARM_GCC_ROOT}/bin")
else()
    set(_TOOLBIN "")
endif()

find_program(CMAKE_C_COMPILER NAMES arm-none-eabi-gcc HINTS ${_TOOLBIN} REQUIRED)
find_program(CMAKE_ASM_COMPILER NAMES arm-none-eabi-gcc HINTS ${_TOOLBIN} REQUIRED)
find_program(CMAKE_OBJCOPY NAMES arm-none-eabi-objcopy HINTS ${_TOOLBIN} REQUIRED)
find_program(CMAKE_SIZE NAMES arm-none-eabi-size HINTS ${_TOOLBIN} REQUIRED)
find_program(CMAKE_OBJDUMP NAMES arm-none-eabi-objdump HINTS ${_TOOLBIN} REQUIRED)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
