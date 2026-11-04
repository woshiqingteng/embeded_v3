#include "FreeRTOS.h"
#include "task.h"

#include "app_init.h"
#include "app_ipc.h"
#include "app_state.h"
#include "elog.h"

#include "task_app_ctrl.h"
#include "task_app_adc.h"
#include "task_app_oled.h"

#define STACK_CTRL 256U
#define STACK_ADC  256U
#define STACK_OLED 1024U

int app_init(void)
{
    app_state_init();
    elog_info("app", "app state ready");

    if (xTaskCreate(task_ctrl, "ctrl", STACK_CTRL, 0, 5, &g_app_ipc.task_ctrl) != pdPASS)
    {
        elog_error("app", "ctrl create failed");
        return -1;
    }
    elog_info("app", "ctrl stack=%u words prio=5", (unsigned)STACK_CTRL);

    if (xTaskCreate(task_adc, "adc", STACK_ADC, 0, 4, &g_app_ipc.task_adc) != pdPASS)
    {
        elog_error("app", "adc create failed");
        return -1;
    }
    elog_info("app", "adc stack=%u words prio=4", (unsigned)STACK_ADC);

    if (xTaskCreate(task_oled, "oled", STACK_OLED, 0, 2, &g_app_ipc.task_oled) != pdPASS)
    {
        elog_error("app", "oled create failed");
        return -1;
    }
    elog_info("app", "oled stack=%u words prio=2", (unsigned)STACK_OLED);

    elog_info("app", "app tasks ready");
    return 0;
}
