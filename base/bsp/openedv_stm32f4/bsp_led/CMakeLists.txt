add_library(bsp_led STATIC bsp_led.c)
target_include_directories(bsp_led PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_led PUBLIC drv_core hal_config)
