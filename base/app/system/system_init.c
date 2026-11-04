#include "FreeRTOS.h"
#include "task.h"

#include "system_init.h"
#include "app_init.h"
#include "app_ipc.h"
#include "app_tick.h"
#include "fault_info.h"

#include "elog.h"
#include "mw_littlefs.h"
#include "mw_log.h"
#include "mw_shell.h"

#include "bsp_init.h"

#include "task_sys_wdg.h"
#include "task_sys_log.h"
#include "task_sys_shell.h"
#include "task_sys_usb.h"

#define STACK_SYSTEM 1024U
#define STACK_WDG    256U
#define STACK_SHELL  768U
#define STACK_LOG    1024U
#define STACK_USB    768U
#define SYSTEM_PRIO  9U
#define WDG_PRIO     12U

static int create_system_tasks(void)
{
    if (xTaskCreate(task_wdg, "wdg", STACK_WDG, 0, WDG_PRIO, &g_app_ipc.task_wdg) != pdPASS) return -1;
    if (xTaskCreate(task_shell, "shell", STACK_SHELL, 0, 4, &g_app_ipc.task_shell) != pdPASS) return -1;
    if (xTaskCreate(task_log, "log", STACK_LOG, 0, 3, &g_app_ipc.task_log) != pdPASS) return -1;
    if (xTaskCreate(task_usb, "usb", STACK_USB, 0, 3, &g_app_ipc.task_usb) != pdPASS) return -1;
    return 0;
}

static void task_system(void *arg)
{
    (void)arg;

    if (bsp_init() != 0)
    {
        elog_error("boot", "bsp init failed");
    }

    if (mw_littlefs_init() == 0 && mw_littlefs_mount() == 0)
    {
        (void)mw_log_init();
    }
    else
    {
        elog_warn("boot", "littlefs not mounted; file log disabled");
    }

    (void)mw_shell_init();

    if (app_ipc_init() == 0)
    {
        if ((create_system_tasks() == 0) && (app_tick_start() == 0) && (app_init() == 0))
        {
            (void)bsp_init_complete();
            elog_info("boot", "BSP + MW + system/app tasks ready");
        }
    }

    vTaskDelete(NULL);
}

int system_init(void)
{
    if (bsp_init_early() != 0)
    {
        return -1;
    }

    (void)elog_init();
    elog_start();

    if (g_fault_info.magic == APP_FAULT_MAGIC)
    {
        elog_error("fault", "cfsr=%08lx hfsr=%08lx pc=%08lx lr=%08lx",
                   (unsigned long)g_fault_info.cfsr,
                   (unsigned long)g_fault_info.hfsr,
                   (unsigned long)g_fault_info.pc,
                   (unsigned long)g_fault_info.lr);
        g_fault_info.magic = 0U;
    }

    if (xTaskCreate(task_system, "system", STACK_SYSTEM, 0, SYSTEM_PRIO, 0) != pdPASS)
    {
        return -1;
    }
    return 0;
}
