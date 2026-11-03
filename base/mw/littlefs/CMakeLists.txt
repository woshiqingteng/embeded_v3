set(TP_LITTLEFS ${THIRD_PARTY}/littlefs/2.11.3)

add_library(littlefs_core OBJECT
    ${TP_LITTLEFS}/lfs.c
    ${TP_LITTLEFS}/lfs_util.c
)
target_include_directories(littlefs_core PUBLIC
    ${TP_LITTLEFS}
    ${INTEGRATION_CONFIG}
)
target_compile_definitions(littlefs_core PUBLIC LFS_DEFINES=lfs_alloc.h)
target_link_libraries(littlefs_core PUBLIC littlefs_config)

add_library(littlefs_port OBJECT
    ${INTEGRATION_BSP}/lfs_bd_nor.c
)
target_include_directories(littlefs_port PUBLIC
    ${TP_LITTLEFS}
    ${INTEGRATION_CONFIG}
    ${INTEGRATION_BSP}
)
target_link_libraries(littlefs_port PUBLIC bsp_nor littlefs_config)

add_library(mw_littlefs_api OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/mw_littlefs.c
)
target_include_directories(mw_littlefs_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TP_LITTLEFS}
)
target_link_libraries(mw_littlefs_api PUBLIC
    littlefs_core
    littlefs_port
    freertos
    littlefs_config
)

add_library(mw_littlefs INTERFACE)
target_sources(mw_littlefs INTERFACE
    $<TARGET_OBJECTS:littlefs_core>
    $<TARGET_OBJECTS:littlefs_port>
    $<TARGET_OBJECTS:mw_littlefs_api>
)
target_link_libraries(mw_littlefs INTERFACE
    littlefs_core
    littlefs_port
    mw_littlefs_api
    littlefs_config
)
