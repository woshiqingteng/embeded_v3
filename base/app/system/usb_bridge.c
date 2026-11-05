#include "usbd_core.h"
#include "mw_usb_bridge.h"
#include "mw_cherryusb.h"
#include "storage_manager.h"

static bool usb_msc_prepare(void)
{
    return (storage_msc_begin() == 0);
}

int mw_usb_bridge_init(void)
{
    mw_cherryusb_set_msc_prepare_cb(usb_msc_prepare);
    return mw_cherryusb_init();
}

void mw_usb_bridge_poll(void)
{
    uint8_t event = mw_cherryusb_take_event();

    if (event == USBD_EVENT_CONFIGURED)
    {
        mw_cherryusb_msc_set_active(true);
    }
    else if ((event == USBD_EVENT_RESET) || (event == USBD_EVENT_DISCONNECTED))
    {
        mw_cherryusb_msc_set_active(false);
        storage_usb_event(event);
    }
}
