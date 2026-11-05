#ifndef MW_CHERRYUSB_H
#define MW_CHERRYUSB_H

#include <stdbool.h>
#include <stdint.h>

typedef bool (*mw_cherryusb_msc_prepare_cb_t)(void);
typedef void (*mw_cherryusb_cdc_rx_cb_t)(const uint8_t *data, uint32_t len);

typedef enum
{
    MW_USB_EVENT_NONE = 0,
    MW_USB_EVENT_CONFIGURED,
    MW_USB_EVENT_RESET,
    MW_USB_EVENT_DISCONNECTED
} mw_usb_event_t;

int  mw_cherryusb_init(void);
void mw_cherryusb_set_msc_prepare_cb(mw_cherryusb_msc_prepare_cb_t cb);
void mw_cherryusb_set_cdc_rx_cb(mw_cherryusb_cdc_rx_cb_t cb);

uint8_t mw_cherryusb_take_event(void);
bool mw_cherryusb_is_configured(void);
void mw_cherryusb_msc_set_active(bool active);
bool mw_cherryusb_cdc_write(const uint8_t *data, uint32_t len);

#endif /* MW_CHERRYUSB_H */
