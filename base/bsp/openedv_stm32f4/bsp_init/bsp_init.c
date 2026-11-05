#include "bsp_init.h"

#include "bsp_core.h"

extern void bsp_core_link_anchor(void);
#include "bsp_log.h"
#include "bsp_uart.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_pwm.h"
#include "bsp_adc.h"
#include "nor.h"
#include "bsp_sd.h"
#include "bsp_internal_flash.h"
#include "bsp_rtc.h"
#include "oled.h"

static void bsp_log_uart_sink(const uint8_t *data, uint32_t len)
{
    (void)bsp_uart_write(BSP_UART1, data, len, 0U);
}

int bsp_init_early(void)
{
    bsp_log_init();
    bsp_core_link_anchor();

    if (bsp_core_init() != 0)
    {
        return -1;
    }

    if (bsp_uart_init() != 0)
    {
        return -1;
    }

    bsp_log_attach_console(bsp_log_uart_sink);
    return 0;
}

int bsp_init(void)
{
    bsp_led_init();
    bsp_key_init();
    bsp_pwm_init();
    bsp_adc_init();

    nor_init();
    (void)bsp_internal_flash_init();
    (void)bsp_rtc_init();
    (void)bsp_sd_init();
    bsp_sd_set_io_hook(bsp_wdg_feed);
    oled_init();
    return 0;
}

int bsp_init_complete(void)
{
    bsp_wdg_init();
    return 0;
}
