set(TP_HAL ${THIRD_PARTY}/stm32_hal/stm32f4xx/1.8.5)
set(HAL_SRC ${TP_HAL}/Src)

add_library(drv_core STATIC
    ${HAL_SRC}/stm32f4xx_hal.c
    ${HAL_SRC}/stm32f4xx_hal_cortex.c
    ${HAL_SRC}/stm32f4xx_hal_rcc.c
    ${HAL_SRC}/stm32f4xx_hal_rcc_ex.c
    ${HAL_SRC}/stm32f4xx_hal_pwr.c
    ${HAL_SRC}/stm32f4xx_hal_pwr_ex.c
    ${HAL_SRC}/stm32f4xx_hal_gpio.c
    ${HAL_SRC}/stm32f4xx_hal_exti.c
    ${HAL_SRC}/stm32f4xx_hal_dma.c
    ${HAL_SRC}/stm32f4xx_hal_dma_ex.c
)

target_include_directories(drv_core SYSTEM PUBLIC ${TP_HAL}/Inc)
target_link_libraries(drv_core PUBLIC cmsis_device hal_config ${MCU_ARCH_INTERFACE})
target_compile_options(drv_core PRIVATE -Wno-unused-parameter -Wno-sign-compare)
