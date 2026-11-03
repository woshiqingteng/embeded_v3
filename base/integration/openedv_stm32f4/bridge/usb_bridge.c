#include "usbd_core.h"
#include "mw_usb_bridge.h"
#include "mw_cherryusb.h"
#include "mw_fatfs.h"
#include "bsp_sd.h"

static volatile bool g_msc_ready;

static bool usb_msc_prepare(void)
{
    if (g_msc_ready)
    {
        return true;
    }

    /* The host is about to use the raw SD card.  Unmount FatFs first so its
     * cached metadata cannot be corrupted by MSC writes. */
    (void)mw_fatfs_umount();

    if (bsp_sd_acquire(BSP_SD_OWNER_MSC, 1000U) != 0)
    {
        return false;
    }

    g_msc_ready = true;
    return true;
}

int mw_usb_bridge_init(void)
{
    (void)mw_fatfs_init();
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
        if (g_msc_ready)
        {
            bsp_sd_release(BSP_SD_OWNER_MSC);
            g_msc_ready = false;
        }
    }
}
