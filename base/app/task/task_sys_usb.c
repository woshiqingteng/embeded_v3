#include "FreeRTOS.h"
#include "task.h"

#include "task_sys_usb.h"
#include "app_ipc.h"
#include "app_state.h"
#include "mw_usb_bridge.h"
#include "mw_cherryusb.h"
#include "rb.h"

static uint8_t  g_cdc_mem[512];
static ringbuf_t g_cdc_rb;

static void usb_cdc_rx(const uint8_t *data, uint32_t len)
{
    (void)rb_put(&g_cdc_rb, data, len);
}

void task_usb(void *arg)
{
    uint8_t buf[64];
    bool init_done = false;

    (void)arg;

    rb_init(&g_cdc_rb, g_cdc_mem, sizeof(g_cdc_mem));
    mw_cherryusb_set_cdc_rx_cb(usb_cdc_rx);
    app_wdg_kick(WDG_BIT_USB);

    if (mw_usb_bridge_init() == 0)
    {
        init_done = true;
    }
    app_wdg_kick(WDG_BIT_USB);

    for (;;)
    {
        if (init_done)
        {
            mw_usb_bridge_poll();
            app_state_set_usb(mw_cherryusb_is_configured());

            uint32_t n = rb_get(&g_cdc_rb, buf, sizeof(buf));
            if (n > 0U)
            {
                (void)mw_cherryusb_cdc_write(buf, n);
            }
        }

        app_wdg_kick(WDG_BIT_USB);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
