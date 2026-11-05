add_library(bsp_oled STATIC oled.c oled_port.c)
target_include_directories(bsp_oled PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_oled PRIVATE drv)
target_link_libraries(bsp_oled PRIVATE os_freertos bsp_log)
