#include "bsp_init.h"

#include "bsp_core.h"
#include "bsp_log.h"
#include "bsp_uart.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_pwm.h"
#include "bsp_adc.h"
#include "nor.h"
#include "bsp_sd.h"
#include "oled.h"

int bsp_init_early(void)
{
    if (bsp_core_init() != HAL_OK)
    {
        return -1;
    }

    bsp_log_init();
    bsp_uart_init();
    bsp_log_attach_uart();
    return 0;
}

int bsp_init(void)
{
    bsp_led_init();
    bsp_key_init();
    bsp_pwm_init();
    bsp_adc_init();

    nor_init();
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
