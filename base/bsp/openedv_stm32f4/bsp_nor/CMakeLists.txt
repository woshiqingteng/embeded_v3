add_library(bsp_nor STATIC nor.c nor_w25q256jv.c spi.c)
target_include_directories(bsp_nor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_nor PUBLIC drv)
