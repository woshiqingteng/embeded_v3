add_library(bsp_adc STATIC bsp_adc.c)
target_include_directories(bsp_adc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_adc PUBLIC drv_adc drv_core hal_config)
