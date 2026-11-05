add_library(mw_lettershell STATIC
    ${TP_LETTER_SHELL}/src/shell.c
    ${TP_LETTER_SHELL}/src/shell_ext.c
    ${BOARD_LETTERSHELL_SOURCES}
)
target_include_directories(mw_lettershell PUBLIC
    ${TP_LETTER_SHELL}/src
    ${BOARD_LETTERSHELL_CONFIG_DIR}
    ${BOARD_PORT_DIR}/lettershell
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(mw_lettershell PRIVATE bsp_uart os_freertos drv)
target_compile_options(mw_lettershell PRIVATE -Wno-unused-parameter -Wno-sign-compare)
