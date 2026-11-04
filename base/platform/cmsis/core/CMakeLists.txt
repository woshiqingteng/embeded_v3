add_library(cmsis_core INTERFACE)
target_include_directories(cmsis_core SYSTEM INTERFACE ${TP_CMSIS_CORE}/Include)
target_link_libraries(cmsis_core INTERFACE ${MCU_ARCH_INTERFACE})
