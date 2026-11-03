add_library(mw_usb_bridge STATIC usb_bridge.c)
target_include_directories(mw_usb_bridge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/mw/cherryusb
    ${CMAKE_SOURCE_DIR}/mw/fatfs
)
target_link_libraries(mw_usb_bridge PUBLIC
    mw_cherryusb
    mw_fatfs
    bsp_sd
    freertos
    integration_config
)
