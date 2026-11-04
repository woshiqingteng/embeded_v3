set(BOARD_NAME openedv_stm32f4)
set(MCU STM32F429IGTx)

include(${CMAKE_CURRENT_LIST_DIR}/../chip/${MCU}.cmake)

set(BOARD_LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/../../platform/linker/openedv_stm32f4/stm32f4xx_flash.ld)
set(BSP_BACKEND openedv_stm32f4)
set(DRV_BACKEND stm32f4_hal)
set(DRV_TARGET  drv_hal)

set(OPENOCD openocd CACHE FILEPATH "openocd executable")
set(OPENOCD_INTERFACE cmsis-dap.cfg CACHE STRING "OpenOCD interface cfg")
set(OPENOCD_TARGET    stm32f4x.cfg  CACHE STRING "OpenOCD target cfg")

set(BOARD_PORT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../port/openedv_stm32f4)
set(BOARD_FREERTOS_CONFIG_DIR   ${BOARD_PORT_DIR}/freertos)
set(BOARD_CHERRYUSB_CONFIG_DIR  ${BOARD_PORT_DIR}/cherryusb)
set(BOARD_FATFS_CONFIG_DIR      ${BOARD_PORT_DIR}/fatfs)
set(BOARD_LITTLEFS_CONFIG_DIR   ${BOARD_PORT_DIR}/littlefs)
set(BOARD_EASYLOGGER_CONFIG_DIR ${BOARD_PORT_DIR}/easylogger)
set(BOARD_LETTERSHELL_CONFIG_DIR ${BOARD_PORT_DIR}/lettershell)

set(BOARD_CHERRYUSB_SOURCES
    ${BOARD_PORT_DIR}/cherryusb/cherryusb_class_cb.c
)
set(BOARD_FATFS_SOURCES
    ${BOARD_PORT_DIR}/fatfs/diskio_sd.c
)
set(BOARD_LITTLEFS_SOURCES
    ${BOARD_PORT_DIR}/littlefs/lfs_bd_nor.c
)
set(BOARD_EASYLOGGER_SOURCES
    ${BOARD_PORT_DIR}/easylogger/elog_port.c
)
set(BOARD_LETTERSHELL_SOURCES
    ${BOARD_PORT_DIR}/lettershell/shell_port.c
)
set(BOARD_FREERTOS_SOURCES)
