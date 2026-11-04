add_library(bsp_usb OBJECT bsp_usb.c)
target_include_directories(bsp_usb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_usb PUBLIC drv)
