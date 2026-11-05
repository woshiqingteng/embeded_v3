add_library(bsp_sd STATIC bsp_sd.c)
target_include_directories(bsp_sd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_sd PUBLIC drv)
target_link_libraries(bsp_sd PRIVATE os_freertos bsp_log)
