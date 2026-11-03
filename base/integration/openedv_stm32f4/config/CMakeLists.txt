add_library(integration_config INTERFACE)
target_include_directories(integration_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# Per-middleware configuration facades.  They are header-only; core/port never
# depend on each other just because they share a configuration header.
add_library(hal_config INTERFACE)
target_link_libraries(hal_config INTERFACE integration_config)

add_library(freertos_config INTERFACE)
target_link_libraries(freertos_config INTERFACE integration_config)

add_library(cherryusb_config INTERFACE)
target_link_libraries(cherryusb_config INTERFACE integration_config)

add_library(fatfs_config INTERFACE)
target_link_libraries(fatfs_config INTERFACE integration_config)

add_library(littlefs_config INTERFACE)
target_link_libraries(littlefs_config INTERFACE integration_config)

add_library(easylogger_config INTERFACE)
target_include_directories(easylogger_config BEFORE INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(easylogger_config INTERFACE integration_config)

add_library(lettershell_config INTERFACE)
target_link_libraries(lettershell_config INTERFACE integration_config)

add_library(lvgl_config INTERFACE)
target_link_libraries(lvgl_config INTERFACE integration_config)
target_compile_definitions(lvgl_config INTERFACE LV_CONF_INCLUDE_SIMPLE)
