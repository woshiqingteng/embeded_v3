add_library(mw_littlefs STATIC
    ${TP_LITTLEFS}/lfs.c
    ${TP_LITTLEFS}/lfs_util.c
    ${BOARD_LITTLEFS_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/mw_littlefs.c
)
target_include_directories(mw_littlefs PUBLIC
    ${TP_LITTLEFS}
    ${BOARD_LITTLEFS_CONFIG_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BOARD_PORT_DIR}/littlefs
)
target_compile_definitions(mw_littlefs PUBLIC LFS_DEFINES=lfs_alloc.h)
target_link_libraries(mw_littlefs PRIVATE bsp_nor os_freertos drv)
target_compile_options(mw_littlefs PRIVATE -Wno-unused-parameter -Wno-sign-compare)
