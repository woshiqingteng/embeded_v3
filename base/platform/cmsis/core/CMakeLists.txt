add_library(cmsis_core INTERFACE)
target_include_directories(cmsis_core SYSTEM INTERFACE
    ${THIRD_PARTY}/cmsis_core/6.4.0/Include
)
target_link_libraries(cmsis_core INTERFACE ${MCU_ARCH_INTERFACE})
