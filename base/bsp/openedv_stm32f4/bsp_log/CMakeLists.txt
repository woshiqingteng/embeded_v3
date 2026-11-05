add_library(bsp_log STATIC bsp_log.c)
target_include_directories(bsp_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_log PRIVATE os_freertos common)
