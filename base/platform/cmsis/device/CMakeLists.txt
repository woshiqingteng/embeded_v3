add_library(cmsis_device INTERFACE)
target_include_directories(cmsis_device INTERFACE ${MCU_CMSIS_DEVICE}/Include)
target_link_libraries(cmsis_device INTERFACE cmsis_core ${MCU_ARCH_INTERFACE})
