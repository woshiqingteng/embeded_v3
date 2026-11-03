set(TP_CUSB ${THIRD_PARTY}/cherryusb/1.6.1)

add_library(cherryusb_core OBJECT
    ${TP_CUSB}/core/usbd_core.c
)
target_include_directories(cherryusb_core PUBLIC
    ${TP_CUSB}
    ${TP_CUSB}/common
    ${TP_CUSB}/core
    ${TP_CUSB}/class
    ${TP_CUSB}/osal
    ${TP_CUSB}/class/hub
    ${TP_CUSB}/class/msc
    ${TP_CUSB}/class/cdc
    ${TP_CUSB}/port/dwc2
)
target_link_libraries(cherryusb_core PUBLIC cherryusb_config)

add_library(cherryusb_class OBJECT
    ${TP_CUSB}/class/msc/usbd_msc.c
    ${TP_CUSB}/class/cdc/usbd_cdc_acm.c
)
target_include_directories(cherryusb_class PUBLIC
    ${TP_CUSB}
    ${TP_CUSB}/common
    ${TP_CUSB}/core
    ${TP_CUSB}/class
    ${TP_CUSB}/class/cdc
    ${TP_CUSB}/class/msc
    ${TP_CUSB}/osal
)
target_link_libraries(cherryusb_class PUBLIC cherryusb_config)

add_library(cherryusb_osal OBJECT
    ${TP_CUSB}/osal/usb_osal_freertos.c
)
target_include_directories(cherryusb_osal PUBLIC
    ${TP_CUSB}
    ${TP_CUSB}/common
    ${TP_CUSB}/core
    ${TP_CUSB}/osal
)
target_link_libraries(cherryusb_osal PUBLIC freertos cherryusb_config)

add_library(cherryusb_port OBJECT
    ${TP_CUSB}/port/dwc2/usb_dc_dwc2.c
    ${TP_CUSB}/port/dwc2/usb_glue_st.c
)
target_include_directories(cherryusb_port PUBLIC
    ${TP_CUSB}
    ${TP_CUSB}/common
    ${TP_CUSB}/core
    ${TP_CUSB}/osal
    ${TP_CUSB}/class
    ${TP_CUSB}/class/hub
    ${TP_CUSB}/class/msc
    ${TP_CUSB}/class/cdc
    ${TP_CUSB}/port/dwc2
    ${INTEGRATION_BSP}
)
target_link_libraries(cherryusb_port PUBLIC
    bsp_usb
    drv_core
    freertos
    cherryusb_config
)

add_library(cherryusb_class_cb OBJECT
    ${INTEGRATION_BSP}/cherryusb_class_cb.c
)
target_include_directories(cherryusb_class_cb PUBLIC
    ${TP_CUSB}
    ${TP_CUSB}/common
    ${TP_CUSB}/core
    ${TP_CUSB}/class
    ${TP_CUSB}/class/cdc
    ${TP_CUSB}/class/msc
    ${INTEGRATION_BSP}
    ${CMAKE_SOURCE_DIR}/mw/cherryusb
)
target_link_libraries(cherryusb_class_cb PUBLIC bsp_sd cherryusb_config)

add_library(mw_cherryusb INTERFACE)
target_sources(mw_cherryusb INTERFACE
    $<TARGET_OBJECTS:cherryusb_core>
    $<TARGET_OBJECTS:cherryusb_class>
    $<TARGET_OBJECTS:cherryusb_osal>
    $<TARGET_OBJECTS:cherryusb_port>
    $<TARGET_OBJECTS:cherryusb_class_cb>
    $<TARGET_OBJECTS:bsp_usb>
)
target_link_libraries(mw_cherryusb INTERFACE
    cherryusb_core
    cherryusb_class
    cherryusb_osal
    cherryusb_port
    cherryusb_class_cb
    bsp_usb
    bsp_sd
    drv_core
    freertos
    cherryusb_config
)
