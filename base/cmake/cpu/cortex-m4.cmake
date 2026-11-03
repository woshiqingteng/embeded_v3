set(CMAKE_SYSTEM_PROCESSOR ${MCU_ARCH})

set(MCU_FLOAT_ABI hard CACHE STRING "FPU ABI (hard|softfp|soft)")
if(NOT MCU_FLOAT_ABI MATCHES "^(hard|softfp|soft)$")
    message(FATAL_ERROR "Invalid MCU_FLOAT_ABI: ${MCU_FLOAT_ABI}")
endif()
message(STATUS "Float ABI : ${MCU_FLOAT_ABI}")

set(MCU_ARCH_FLAGS -mcpu=${MCU_ARCH} -mthumb -mfloat-abi=${MCU_FLOAT_ABI})
if(NOT MCU_FLOAT_ABI STREQUAL "soft")
    list(APPEND MCU_ARCH_FLAGS -mfpu=fpv4-sp-d16)
endif()

set(MCU_ARCH_INTERFACE cpu_${MCU_ARCH} CACHE INTERNAL "" FORCE)

add_library(${MCU_ARCH_INTERFACE} INTERFACE)
target_compile_options(${MCU_ARCH_INTERFACE} INTERFACE ${MCU_ARCH_FLAGS})
target_link_options(${MCU_ARCH_INTERFACE}    INTERFACE ${MCU_ARCH_FLAGS})
