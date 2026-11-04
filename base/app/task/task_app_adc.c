#include "FreeRTOS.h"
#include "task.h"

#include "task_app_adc.h"
#include "app_ipc.h"
#include "app_state.h"
#include "bsp_adc.h"

void task_adc(void *arg)
{
    (void)arg;

    for (;;)
    {
        uint16_t mv[6];

        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));
        if (bsp_adc_get_mv(mv))
        {
            app_state_set_adc(mv);
        }

        app_wdg_kick(WDG_BIT_ADC);
    }
}
