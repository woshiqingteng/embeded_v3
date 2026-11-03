#ifndef USB_CONFIG_H
#define USB_CONFIG_H

#include <stdio.h>

#define CONFIG_USB_PRINTF(...) printf(__VA_ARGS__)
#define CONFIG_USB_DBG_LEVEL   USB_DBG_ERROR

#define USB_NOCACHE_RAM_SECTION
#define CONFIG_USB_ALIGN_SIZE   4

/* Device stack */
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 512
#define CONFIG_USBDEV_ADVANCE_DESC
#define CONFIG_USBDEV_MSC_THREAD
#define CONFIG_USBDEV_MSC_MAX_LUN       1
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE   512
#define CONFIG_USBDEV_MSC_PRIO          11
#define CONFIG_USBDEV_MSC_STACKSIZE     4096
#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING "ALIENTEK"
#define CONFIG_USBDEV_MSC_PRODUCT_STRING      "STM32F4 MSC"
#define CONFIG_USBDEV_MSC_VERSION_STRING      "1.00"
#define CONFIG_USBDEV_MAX_BUS           1

/* DWC2 FS: PA11/PA12 is slave-only, no DMA mode. */
/* #define CONFIG_USB_DWC2_DMA_ENABLE */

/* Host geometry is required at compile time by usb_glue_st.c (usbh_core.h),
 * even though no host source is built in this project. */
#define CONFIG_USBHOST_MAX_RHPORTS          1
#define CONFIG_USBHOST_MAX_EXTHUBS          1
#define CONFIG_USBHOST_MAX_EHPORTS          4
#define CONFIG_USBHOST_MAX_INTERFACES       8
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 2
#define CONFIG_USBHOST_MAX_ENDPOINTS        4
#define CONFIG_USBHOST_DEV_NAMELEN          16
#define CONFIG_USBHOST_REQUEST_BUFFER_LEN   512
#define CONFIG_USBHOST_MAX_BUS              1

#define usb_phyaddr2ramaddr(addr) (addr)
#define usb_ramaddr2phyaddr(addr) (addr)

#endif /* USB_CONFIG_H */
