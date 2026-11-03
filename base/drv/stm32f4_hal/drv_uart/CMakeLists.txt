set(TP_HAL ${THIRD_PARTY}/stm32_hal/stm32f4xx/1.8.5)
add_library(drv_uart STATIC ${TP_HAL}/Src/stm32f4xx_hal_uart.c)
target_link_libraries(drv_uart PUBLIC drv_core)
target_compile_options(drv_uart PRIVATE -Wno-unused-parameter -Wno-sign-compare)
