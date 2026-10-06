# Post-project() fixups for the armclang toolchain.
#
# During project() CMake's Compiler/ARMClang.cmake module:
#   * sets CMAKE_EXECUTABLE_SUFFIX ".elf" (we already name the target *.elf), and
#   * replaces CMAKE_C_LINK_EXECUTABLE with an armlink-style rule that appends
#     "-Xlinker --list=<TARGET_BASE>.map" (GNU ld has no --list option).
# We link with the GNU gcc driver + our .ld script, so restore the GNU-style
# link rule and clear the suffix. Include this file after project().
#
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/armclang-postproject.cmake)

set(CMAKE_EXECUTABLE_SUFFIX "")

# <FLAGS> is the C compile flags (contains --target=, -include, ...) which the
# GNU driver must not see when linking; the cpu/arch/opt flags live in
# LINK_FLAGS (set by xmc1100_board.cmake).
set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_LINKER> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")