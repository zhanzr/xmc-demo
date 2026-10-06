# Shared flashing targets for the XMC4500-F100x1024 "xmc4500-relax-lite" board.
# Programming is done with SEGGER J-Link over SWD; the same J-Link debugger
# provides the console UART on P1.5/P1.4 (onboard VCOM).
#
# Targets:
#   ninja flash        - J-Link Commander script download (.hex), reset, run
#   ninja flash-bin    - same, but downloads the raw .bin at 0x08000000
#   ninja erase        - chip erase via J-Link
#
# Overrides:
#   -DSEGGER_JLINK_EXE=/path/to/JLink.exe
#   -DJLINK_DEVICE=XMC4500-1024      (device name as known to JLink)
#   -DJLINK_IF=SWD  -DJLINK_SPEED=4000

find_program(SEGGER_JLINK_EXE NAMES JLink JLink.exe
    HINTS "D:/Program Files/SEGGER/JLink_V956"
          "C:/Program Files/SEGGER/JLink"
          "C:/Program Files (x86)/SEGGER/JLink"
    DOC "SEGGER JLink.exe (programmer used by this board)")

set(JLINK_DEVICE "XMC4500-1024" CACHE STRING "J-Link device name for this board")
set(JLINK_IF "SWD" CACHE STRING "J-Link interface (SWD or JTAG)")
set(JLINK_SPEED "4000" CACHE STRING "J-Link SWD/JTAG speed in kHz")

# Build products live in the project build/ folder (git-ignored). TARGET_FILE keeps
# this correct even though the two toolchains disagree on CMAKE_EXECUTABLE_SUFFIX.
set(BIN_ELF "$<TARGET_FILE:${PROJECT_NAME}.elf>")
set(BIN_HEX "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex")
set(BIN_BIN "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.bin")

# XMC4500 flash is mapped cached at 0x08000000 (uncached alias 0x0C000000).
set(XMC_FLASH_ORIGIN "0x08000000")

add_custom_command(OUTPUT ${BIN_HEX} ${BIN_BIN}
    COMMAND ${CMAKE_OBJCOPY} -O ihex   "${BIN_ELF}" "${BIN_HEX}"
    COMMAND ${CMAKE_OBJCOPY} -O binary "${BIN_ELF}" "${BIN_BIN}"
    COMMAND ${CMAKE_SIZE} "${BIN_ELF}"
    DEPENDS ${PROJECT_NAME}.elf
    VERBATIM
    COMMENT "objcopy -> .hex/.bin (in build/), size")

# Part of the default `all` build, so plain `ninja` also emits .hex/.bin.
add_custom_target(${PROJECT_NAME}_hex ALL DEPENDS ${BIN_HEX} ${BIN_BIN})

if(SEGGER_JLINK_EXE)
    set(_JLINK_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/jlink_flash.jlink")
    set(_JLINK_SCRIPT_BIN "${CMAKE_CURRENT_BINARY_DIR}/jlink_flash_bin.jlink")
    set(_JLINK_SCRIPT_ERASE "${CMAKE_CURRENT_BINARY_DIR}/jlink_erase.jlink")

    file(WRITE "${_JLINK_SCRIPT}"
"r
h
loadfile ${BIN_HEX}
r
g
q
")
    file(WRITE "${_JLINK_SCRIPT_BIN}"
"r
h
loadbin ${BIN_BIN} ${XMC_FLASH_ORIGIN}
r
g
q
")
    file(WRITE "${_JLINK_SCRIPT_ERASE}"
"r
h
erase
q
")

    add_custom_target(flash
        COMMAND "${SEGGER_JLINK_EXE}" -device "${JLINK_DEVICE}" -if "${JLINK_IF}"
                    -speed "${JLINK_SPEED}" -autoconnect 1 -nogui 1
                    -CommanderScript "${_JLINK_SCRIPT}"
        DEPENDS ${BIN_HEX}
        COMMENT "Flashing ${PROJECT_NAME}.hex to ${JLINK_DEVICE} via J-Link (${JLINK_IF}) ..."
        USES_TERMINAL)

    add_custom_target(flash-bin
        COMMAND "${SEGGER_JLINK_EXE}" -device "${JLINK_DEVICE}" -if "${JLINK_IF}"
                    -speed "${JLINK_SPEED}" -autoconnect 1 -nogui 1
                    -CommanderScript "${_JLINK_SCRIPT_BIN}"
        DEPENDS ${BIN_BIN}
        COMMENT "Flashing ${PROJECT_NAME}.bin to ${JLINK_DEVICE} at ${XMC_FLASH_ORIGIN} ..."
        USES_TERMINAL)

    add_custom_target(erase
        COMMAND "${SEGGER_JLINK_EXE}" -device "${JLINK_DEVICE}" -if "${JLINK_IF}"
                    -speed "${JLINK_SPEED}" -autoconnect 1 -nogui 1
                    -CommanderScript "${_JLINK_SCRIPT_ERASE}"
        COMMENT "Erasing ${JLINK_DEVICE} ..."
        USES_TERMINAL)
else()
    foreach(_tgt flash flash-bin erase)
        add_custom_target(${_tgt}
            COMMAND ${CMAKE_COMMAND} -E echo
                "JLink.exe not found. Install SEGGER J-Link or pass -DSEGGER_JLINK_EXE=/path/to/JLink.exe"
            USES_TERMINAL)
    endforeach()
endif()
