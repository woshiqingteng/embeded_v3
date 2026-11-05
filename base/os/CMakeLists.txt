add_subdirectory(freertos)

add_library(os INTERFACE)
target_link_libraries(os INTERFACE os_freertos)
