add_library(bsp_init STATIC bsp_init.c)
target_include_directories(bsp_init PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_init PUBLIC
    bsp_core
    bsp_log
    bsp_uart
    bsp_led
    bsp_key
    bsp_pwm
    bsp_adc
    bsp_nor
    bsp_sd
    bsp_oled
    os_freertos
    drv
)
