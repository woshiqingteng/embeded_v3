add_library(cmsis_startup OBJECT
    startup_stm32f4xx.c
    vector_stm32f4xx.c
    system_stm32f4xx.c
    stm32f4xx_it.c
)
target_include_directories(cmsis_startup PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(cmsis_startup PUBLIC cmsis_device)
