add_library(mw_easylogger STATIC
    ${TP_EASYLOGGER}/src/elog.c
    ${TP_EASYLOGGER}/src/elog_utils.c
    ${BOARD_EASYLOGGER_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/log_file.c
)
target_include_directories(mw_easylogger BEFORE PUBLIC ${BOARD_EASYLOGGER_CONFIG_DIR})
target_include_directories(mw_easylogger PUBLIC
    ${TP_EASYLOGGER}/inc
    ${BOARD_PORT_DIR}/easylogger
    ${BOARD_PORT_DIR}/littlefs
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/mw/littlefs
)
target_link_libraries(mw_easylogger PUBLIC
    bsp_log
    bsp_uart
    mw_littlefs
    os_freertos
    drv
)
target_compile_options(mw_easylogger PRIVATE -Wno-unused-parameter -Wno-sign-compare)
