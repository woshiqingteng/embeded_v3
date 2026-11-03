set(TP_HAL ${THIRD_PARTY}/stm32_hal/stm32f4xx/1.8.5)
add_library(drv_spi STATIC ${TP_HAL}/Src/stm32f4xx_hal_spi.c)
target_link_libraries(drv_spi PUBLIC drv_core)
target_compile_options(drv_spi PRIVATE -Wno-unused-parameter -Wno-sign-compare)
