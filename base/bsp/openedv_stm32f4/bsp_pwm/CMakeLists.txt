add_library(bsp_pwm STATIC bsp_pwm.c)
target_include_directories(bsp_pwm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bsp_pwm PRIVATE drv bsp_log bsp_core)
