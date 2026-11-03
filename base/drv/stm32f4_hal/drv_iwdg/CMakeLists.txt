set(TP_HAL ${THIRD_PARTY}/stm32_hal/stm32f4xx/1.8.5)
add_library(drv_iwdg STATIC ${TP_HAL}/Src/stm32f4xx_hal_iwdg.c)
target_link_libraries(drv_iwdg PUBLIC drv_core)
target_compile_options(drv_iwdg PRIVATE -Wno-unused-parameter -Wno-sign-compare)
