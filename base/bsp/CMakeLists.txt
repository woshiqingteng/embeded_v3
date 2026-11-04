if(NOT BSP_BACKEND)
    message(FATAL_ERROR "BSP_BACKEND is not set by board: ${BOARD}")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${BSP_BACKEND}/CMakeLists.txt")
    message(FATAL_ERROR "Unsupported BSP_BACKEND: ${BSP_BACKEND}")
endif()

add_subdirectory(${BSP_BACKEND})
