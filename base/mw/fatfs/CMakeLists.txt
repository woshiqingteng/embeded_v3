set(TP_FATFS ${THIRD_PARTY}/fatfs/r0.16)

add_library(fatfs_core OBJECT
    ${TP_FATFS}/ff.c
    ${TP_FATFS}/ffunicode.c
)
target_include_directories(fatfs_core PUBLIC
    ${TP_FATFS}
    ${INTEGRATION_CONFIG}
)
target_link_libraries(fatfs_core PUBLIC fatfs_config)

add_library(fatfs_port OBJECT
    ${INTEGRATION_BSP}/diskio_sd.c
)
target_include_directories(fatfs_port PUBLIC
    ${TP_FATFS}
    ${INTEGRATION_CONFIG}
    ${INTEGRATION_BSP}
)
target_link_libraries(fatfs_port PUBLIC bsp_sd fatfs_config)

add_library(mw_fatfs_api OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/mw_fatfs.c
)
target_include_directories(mw_fatfs_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TP_FATFS}
)
target_link_libraries(mw_fatfs_api PUBLIC
    fatfs_core
    bsp_sd
    freertos
    fatfs_config
)

add_library(mw_fatfs INTERFACE)
target_sources(mw_fatfs INTERFACE
    $<TARGET_OBJECTS:fatfs_core>
    $<TARGET_OBJECTS:fatfs_port>
    $<TARGET_OBJECTS:mw_fatfs_api>
)
target_link_libraries(mw_fatfs INTERFACE
    fatfs_core
    fatfs_port
    mw_fatfs_api
    fatfs_config
)
