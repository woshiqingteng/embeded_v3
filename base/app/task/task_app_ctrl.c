#include "FreeRTOS.h"
#include "task.h"

#include "task_app_ctrl.h"
#include "app_ipc.h"
#include "app_state.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "bsp_pwm.h"

void task_ctrl(void *arg)
{
    (void)arg;

    for (;;)
    {
        uint32_t value = 0U;

        if (xTaskNotifyWait(0U, 0xFFFFFFFFU, &value,
                            pdMS_TO_TICKS(100)) == pdTRUE)
        {
            app_state_set_key(value);

            switch (value)
            {
                case BSP_KEY0:
                    bsp_led_toggle(BSP_LED1);
                    break;

                case BSP_KEY1:
                {
                    uint16_t duty = bsp_pwm_get_duty();
                    duty = (duty <= 900U) ? (uint16_t)(duty + 100U) : 1000U;
                    bsp_pwm_set_duty(duty);
                    app_state_set_pwm(duty);
                    break;
                }

                case BSP_KEY2:
                {
                    uint16_t duty = bsp_pwm_get_duty();
                    duty = (duty >= 100U) ? (uint16_t)(duty - 100U) : 0U;
                    bsp_pwm_set_duty(duty);
                    app_state_set_pwm(duty);
                    break;
                }

                case BSP_KEY_WKUP:
                    bsp_led_on(BSP_LED1);
                    bsp_pwm_set_duty(500U);
                    app_state_set_pwm(500U);
                    break;

                default:
                    break;
            }
        }

        app_wdg_kick(WDG_BIT_CTRL);
    }
}
