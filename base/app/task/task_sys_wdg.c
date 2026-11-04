#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#include "task_sys_wdg.h"
#include "app_ipc.h"
#include "bsp_core.h"

void task_wdg(void *arg)
{
    (void)arg;

    for (;;)
    {
        EventBits_t bits = xEventGroupWaitBits(g_app_ipc.wdg_event,
                                               WDG_BIT_ALL,
                                               pdTRUE, pdTRUE,
                                               pdMS_TO_TICKS(100));

        if ((bits & WDG_BIT_ALL) == WDG_BIT_ALL)
        {
            bsp_wdg_feed();
        }
    }
}
