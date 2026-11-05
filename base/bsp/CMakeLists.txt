if(NOT BSP_BACKEND)
    message(FATAL_ERROR "BSP_BACKEND is not set by board: ${BOARD}")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${BSP_BACKEND}/CMakeLists.txt")
    message(FATAL_ERROR "Unsupported BSP_BACKEND: ${BSP_BACKEND}")
endif()

add_subdirectory(${BSP_BACKEND})

add_library(bsp INTERFACE)
target_link_libraries(bsp INTERFACE
    bsp_init
    bsp_core
    bsp_runtime_stats
    bsp_internal_flash
    bsp_rtc
    bsp_uart
    bsp_log
    bsp_led
    bsp_key
    bsp_pwm
    bsp_adc
    bsp_oled
    bsp_sd
    bsp_nor
)
