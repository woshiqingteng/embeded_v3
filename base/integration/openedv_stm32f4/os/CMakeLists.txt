add_library(integration_os STATIC
    freertos_glue.c
    runtime_stats_tim7.c
)
target_include_directories(integration_os PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(integration_os PUBLIC
    freertos
    drv_core
    drv_tim
    integration_config
)
