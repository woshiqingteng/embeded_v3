add_library(cmsis_device OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/startup_stm32f4xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/vector_stm32f4xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/system_stm32f4xx.c
    ${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syscalls.c
)

target_include_directories(cmsis_device PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${THIRD_PARTY}/cmsis_device/stm32f4xx/2.6.11/Include
)

target_link_libraries(cmsis_device PUBLIC cmsis_core ${MCU_ARCH_INTERFACE})

target_link_options(cmsis_device INTERFACE
    "-T${CMAKE_CURRENT_SOURCE_DIR}/stm32f4xx_flash.ld"
    "-Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map"
)
