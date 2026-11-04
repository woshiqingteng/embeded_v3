#include "FreeRTOS.h"
#include "task.h"

#include "app_init.h"
#include "app_ipc.h"
#include "app_state.h"

#include "task_app_ctrl.h"
#include "task_app_adc.h"
#include "task_app_oled.h"

#define STACK_CTRL 256U
#define STACK_ADC  256U
#define STACK_OLED 1024U

int app_init(void)
{
    app_state_init();

    if (xTaskCreate(task_ctrl, "ctrl", STACK_CTRL, 0, 5, &g_app_ipc.task_ctrl) != pdPASS) return -1;
    if (xTaskCreate(task_adc, "adc", STACK_ADC, 0, 4, &g_app_ipc.task_adc) != pdPASS) return -1;
    if (xTaskCreate(task_oled, "oled", STACK_OLED, 0, 2, &g_app_ipc.task_oled) != pdPASS) return -1;
    return 0;
}
