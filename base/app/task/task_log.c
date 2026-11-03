#include "FreeRTOS.h"
#include "task.h"

#include "task_log.h"
#include "app_ipc.h"
#include "bsp_log.h"

void task_log(void *arg)
{
    (void)arg;

    for (;;)
    {
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
        bsp_log_poll();
        app_wdg_kick(WDG_BIT_LOG);
    }
}
