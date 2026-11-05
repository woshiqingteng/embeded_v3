cmake_minimum_required(VERSION 3.24)

if(NOT TOOLCHAIN)
    set(TOOLCHAIN arm_gcc)
endif()
include(${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain/${TOOLCHAIN}.cmake)
message(STATUS "Toolchain : ${TOOLCHAIN}")

if(NOT BOARD)
    set(BOARD openedv_stm32f4)
endif()
include(${CMAKE_CURRENT_LIST_DIR}/cmake/third_party/dependencies.cmake)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/board/${BOARD}.cmake)
message(STATUS "Board : ${BOARD}")
include(${CMAKE_CURRENT_LIST_DIR}/cmake/func/flow.cmake)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

project(firmware LANGUAGES C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_definitions(
    ${MCU_MACRO}
    HSE_VALUE=${MCU_HSE_VALUE}
    HSI_VALUE=${MCU_HSI_VALUE}
    USE_FREERTOS=1
)

add_subdirectory(platform)
add_subdirectory(common)
add_subdirectory(drv)
add_subdirectory(os)
add_subdirectory(bsp)
add_subdirectory(mw)
add_subdirectory(app)

