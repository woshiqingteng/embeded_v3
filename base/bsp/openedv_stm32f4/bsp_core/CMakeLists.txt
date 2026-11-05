add_library(bsp_core STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_core.c
)

target_include_directories(bsp_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_core PUBLIC drv cmsis_device)
target_link_libraries(bsp_core PRIVATE bsp_log)

add_library(bsp_runtime_stats STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_runtime_stats.c
)
target_include_directories(bsp_runtime_stats PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_runtime_stats PRIVATE drv)
