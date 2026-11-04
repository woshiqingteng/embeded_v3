if(NOT MCU_FAMILY)
    message(FATAL_ERROR "MCU_FAMILY is not set by chip")
endif()

add_subdirectory(${MCU_FAMILY})
