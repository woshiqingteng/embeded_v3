add_library(bsp_rtc STATIC bsp_rtc.c)
target_include_directories(bsp_rtc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_rtc PRIVATE drv bsp_log os_freertos)
