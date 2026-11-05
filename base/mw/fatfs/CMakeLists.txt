add_library(mw_fatfs STATIC
    ${TP_FATFS}/ff.c
    ${TP_FATFS}/ffunicode.c
    ${BOARD_FATFS_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/mw_fatfs.c
)
target_include_directories(mw_fatfs PUBLIC
    ${TP_FATFS}
    ${BOARD_FATFS_CONFIG_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(mw_fatfs PRIVATE bsp_sd os_freertos drv)
target_compile_options(mw_fatfs PRIVATE -Wno-unused-parameter -Wno-sign-compare)
