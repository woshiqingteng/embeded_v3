if(NOT BOARD_LINKER_SCRIPT OR NOT EXISTS "${BOARD_LINKER_SCRIPT}")
    message(FATAL_ERROR "BOARD_LINKER_SCRIPT is not set by board: ${BOARD}")
endif()

add_library(platform_linker INTERFACE)
target_link_options(platform_linker INTERFACE
    "-T${BOARD_LINKER_SCRIPT}"
    "-Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map"
)
