add_library(bsp_sd STATIC sdio.c bsp_sd.c)
target_include_directories(bsp_sd PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_sd PUBLIC drv os_freertos)
