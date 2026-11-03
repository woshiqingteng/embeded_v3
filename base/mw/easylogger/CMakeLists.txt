set(TP_ELOG ${THIRD_PARTY}/easy_logger/2.2.0)

add_library(easylogger_core OBJECT
    ${TP_ELOG}/src/elog.c
    ${TP_ELOG}/src/elog_utils.c
)
target_include_directories(easylogger_core BEFORE PUBLIC ${INTEGRATION_CONFIG})
target_include_directories(easylogger_core PUBLIC ${TP_ELOG}/inc)
target_link_libraries(easylogger_core PUBLIC easylogger_config)

add_library(easylogger_port OBJECT
    ${INTEGRATION_BSP}/elog_port.c
)
target_include_directories(easylogger_port BEFORE PUBLIC ${INTEGRATION_CONFIG})
target_include_directories(easylogger_port PUBLIC
    ${TP_ELOG}/inc
    ${INTEGRATION_BSP}
)
target_link_libraries(easylogger_port PUBLIC
    bsp_log
    bsp_uart
    freertos
    drv_core
    easylogger_config
)

add_library(easylogger_log_file OBJECT
    ${INTEGRATION_BRIDGE}/log_file.c
)
target_include_directories(easylogger_log_file PUBLIC
    ${CMAKE_SOURCE_DIR}/mw/easylogger
    ${CMAKE_SOURCE_DIR}/mw/littlefs
    ${INTEGRATION_CONFIG}
)
target_link_libraries(easylogger_log_file PUBLIC
    mw_littlefs
    bsp_log
    freertos
    easylogger_config
)

add_library(mw_easylogger INTERFACE)
target_sources(mw_easylogger INTERFACE
    $<TARGET_OBJECTS:easylogger_core>
    $<TARGET_OBJECTS:easylogger_port>
    $<TARGET_OBJECTS:easylogger_log_file>
)
target_link_libraries(mw_easylogger INTERFACE
    easylogger_core
    easylogger_port
    easylogger_log_file
    bsp_log
    mw_littlefs
    easylogger_config
)
