#include "FreeRTOS.h"
#include "task.h"

#include "task_usb.h"
#include "app_ipc.h"
#include "app_state.h"
#include "mw_usb_bridge.h"
#include "mw_cherryusb.h"
#include "usbd_core.h"
#include "ringbuf.h"

static uint8_t  g_cdc_mem[512];
static ringbuf_t g_cdc_rb;

static void usb_cdc_rx(const uint8_t *data, uint32_t len)
{
    (void)ringbuf_put(&g_cdc_rb, data, len);
}

void task_usb(void *arg)
{
    uint8_t buf[64];
    bool init_done = false;

    (void)arg;

    ringbuf_init(&g_cdc_rb, g_cdc_mem, sizeof(g_cdc_mem));
    mw_cherryusb_set_cdc_rx_cb(usb_cdc_rx);

    if (mw_usb_bridge_init() == 0)
    {
        init_done = true;
    }

    for (;;)
    {
        if (init_done)
        {
            mw_usb_bridge_poll();
            app_state_set_usb(usb_device_is_configured(0));

            uint32_t n = ringbuf_get(&g_cdc_rb, buf, sizeof(buf));
            if (n > 0U)
            {
                (void)mw_cherryusb_cdc_write(buf, n);
            }
        }

        app_wdg_kick(WDG_BIT_USB);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
