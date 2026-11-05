add_library(mw_easyflash STATIC
    ${TP_EASYFLASH}/src/easyflash.c
    ${TP_EASYFLASH}/src/ef_log.c
    ${TP_EASYFLASH}/src/ef_utils.c
    ${BOARD_PORT_DIR}/easyflash/ef_port.c
)
target_include_directories(mw_easyflash PUBLIC
    ${TP_EASYFLASH}/inc
    ${BOARD_PORT_DIR}/easyflash
)
target_link_libraries(mw_easyflash PRIVATE
    bsp_internal_flash
    os_freertos
    drv
)
target_compile_options(mw_easyflash PRIVATE -Wno-unused-parameter)
