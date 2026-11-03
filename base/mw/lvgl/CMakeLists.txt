option(ENABLE_LVGL "Build LVGL middleware" OFF)

add_library(mw_lvgl INTERFACE)
target_link_libraries(mw_lvgl INTERFACE lvgl_config)

if(ENABLE_LVGL)
    message(STATUS "LVGL is enabled but no display port is linked in this firmware")
endif()
