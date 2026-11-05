add_library(bsp_nor STATIC nor.c)
target_include_directories(bsp_nor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_nor PRIVATE drv bsp_log os_freertos)
