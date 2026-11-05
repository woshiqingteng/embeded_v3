add_library(bsp_uart STATIC bsp_uart.c)
target_include_directories(bsp_uart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_uart PRIVATE drv os_freertos common bsp_log)
