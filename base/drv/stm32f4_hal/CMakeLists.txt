set(HAL_SRC ${TP_STM32_HAL}/Src)
add_library(drv_hal STATIC
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
    ${HAL_SRC}/stm32f4xx_hal_adc.c
    ${HAL_SRC}/stm32f4xx_hal_adc_ex.c
    ${HAL_SRC}/stm32f4xx_hal_spi.c
    ${HAL_SRC}/stm32f4xx_hal_tim.c
    ${HAL_SRC}/stm32f4xx_hal_tim_ex.c
    ${HAL_SRC}/stm32f4xx_hal_uart.c
    ${HAL_SRC}/stm32f4xx_hal_iwdg.c
    ${HAL_SRC}/stm32f4xx_hal_flash.c
    ${HAL_SRC}/stm32f4xx_hal_flash_ex.c
    ${HAL_SRC}/stm32f4xx_hal_rtc.c
    ${HAL_SRC}/stm32f4xx_hal_rtc_ex.c
    ${HAL_SRC}/stm32f4xx_hal_sd.c
    ${HAL_SRC}/stm32f4xx_ll_sdmmc.c
    ${HAL_SRC}/stm32f4xx_hal_pcd.c
    ${HAL_SRC}/stm32f4xx_hal_pcd_ex.c
    ${HAL_SRC}/stm32f4xx_ll_usb.c
)
target_include_directories(drv_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TP_STM32_HAL}/Inc
)
target_compile_definitions(drv_hal PUBLIC
    HAL_MODULE_ENABLED
    HAL_ADC_MODULE_ENABLED
    HAL_CORTEX_MODULE_ENABLED
    HAL_DMA_MODULE_ENABLED
    HAL_EXTI_MODULE_ENABLED
    HAL_GPIO_MODULE_ENABLED
    HAL_IWDG_MODULE_ENABLED
    HAL_PCD_MODULE_ENABLED
    HAL_PWR_MODULE_ENABLED
    HAL_RCC_MODULE_ENABLED
    HAL_SD_MODULE_ENABLED
    HAL_SPI_MODULE_ENABLED
    HAL_TIM_MODULE_ENABLED
    HAL_UART_MODULE_ENABLED
)
target_link_libraries(drv_hal PUBLIC cmsis_device ${MCU_ARCH_INTERFACE})
target_compile_options(drv_hal PRIVATE -Wno-unused-parameter -Wno-sign-compare)
