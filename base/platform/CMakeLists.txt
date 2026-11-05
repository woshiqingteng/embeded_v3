add_subdirectory(cmsis/core)
add_subdirectory(cmsis/device)
add_subdirectory(startup)
add_subdirectory(linker)

add_library(platform INTERFACE)
target_link_libraries(platform INTERFACE
    cmsis_core
    cmsis_device
    cmsis_startup
    platform_linker
)
