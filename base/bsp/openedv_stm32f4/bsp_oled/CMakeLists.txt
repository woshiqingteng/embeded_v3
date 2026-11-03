add_library(bsp_oled STATIC oled.c oled_ssd1306.c)
target_include_directories(bsp_oled PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/bsp/openedv_stm32f4/bsp_core
)
target_link_libraries(bsp_oled PUBLIC drv_spi drv_core hal_config)
