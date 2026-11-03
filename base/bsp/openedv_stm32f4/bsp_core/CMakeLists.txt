add_library(bsp_core OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_timebase.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_wdg.c
)

target_include_directories(bsp_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(bsp_core PUBLIC
    drv_core
    drv_tim
    drv_iwdg
    cmsis_device
    hal_config
)
