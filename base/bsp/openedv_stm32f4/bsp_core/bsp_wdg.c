#include "stm32f4xx_hal.h"
#include "bsp_core.h"

static IWDG_HandleTypeDef g_iwdg;

void bsp_wdg_init(void)
{
    g_iwdg.Instance       = IWDG;
    g_iwdg.Init.Prescaler = IWDG_PRESCALER_64; /* LSI ~32 kHz */
    g_iwdg.Init.Reload    = 2000U;             /* ~4 s */
    (void)HAL_IWDG_Init(&g_iwdg);
}

void bsp_wdg_feed(void)
{
    (void)HAL_IWDG_Refresh(&g_iwdg);
}
