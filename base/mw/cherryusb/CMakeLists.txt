add_library(mw_cherryusb STATIC
    ${TP_CHERRYUSB}/core/usbd_core.c
    ${TP_CHERRYUSB}/class/msc/usbd_msc.c
    ${TP_CHERRYUSB}/class/cdc/usbd_cdc_acm.c
    ${TP_CHERRYUSB}/osal/usb_osal_freertos.c
    ${TP_CHERRYUSB}/port/dwc2/usb_dc_dwc2.c
    ${TP_CHERRYUSB}/port/dwc2/usb_glue_st.c
    ${BOARD_CHERRYUSB_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/usb_bridge.c
)
target_include_directories(mw_cherryusb PUBLIC
    ${TP_CHERRYUSB}
    ${TP_CHERRYUSB}/common
    ${TP_CHERRYUSB}/core
    ${TP_CHERRYUSB}/class
    ${TP_CHERRYUSB}/class/hub
    ${TP_CHERRYUSB}/class/msc
    ${TP_CHERRYUSB}/class/cdc
    ${TP_CHERRYUSB}/osal
    ${TP_CHERRYUSB}/port/dwc2
    ${BOARD_CHERRYUSB_CONFIG_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${BOARD_PORT_DIR}/cherryusb
)
target_link_libraries(mw_cherryusb PUBLIC
    bsp_usb
    bsp_sd
    mw_fatfs
    os_freertos
    drv
)
target_compile_options(mw_cherryusb PRIVATE -Wno-unused-parameter -Wno-sign-compare)
