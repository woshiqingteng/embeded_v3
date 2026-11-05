add_library(bsp_key STATIC bsp_key.c)
target_include_directories(bsp_key PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_key PUBLIC common)
target_link_libraries(bsp_key PRIVATE drv bsp_log)
