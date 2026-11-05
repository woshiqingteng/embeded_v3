#include "FreeRTOS.h"
#include "task.h"

#include "system_init.h"
#include "app_init.h"
#include "app_ipc.h"
#include "app_tick.h"
#include "fault_info.h"

#include "elog.h"
#include "fs.h"
#include "mw_log.h"
#include "mw_shell.h"
#include "storage_manager.h"

#include "bsp_init.h"
#include "bsp_log.h"
#include "bsp_uart.h"

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
    if (xTaskCreate(task_wdg, "wdg", STACK_WDG, 0, WDG_PRIO, &g_app_ipc.task_wdg) != pdPASS)
    {
        return -1;
    }
    elog_info("sys", "wdg stack=%u words prio=%u", (unsigned)STACK_WDG, (unsigned)WDG_PRIO);
    if (xTaskCreate(task_shell, "shell", STACK_SHELL, 0, 4, &g_app_ipc.task_shell) != pdPASS)
    {
        vTaskDelete(g_app_ipc.task_wdg);
        g_app_ipc.task_wdg = 0;
        return -1;
    }
    elog_info("sys", "shell stack=%u words prio=4", (unsigned)STACK_SHELL);
    if (xTaskCreate(task_log, "log", STACK_LOG, 0, 3, &g_app_ipc.task_log) != pdPASS)
    {
        vTaskDelete(g_app_ipc.task_shell);
        vTaskDelete(g_app_ipc.task_wdg);
        g_app_ipc.task_shell = 0;
        g_app_ipc.task_wdg = 0;
        return -1;
    }
    elog_info("sys", "log stack=%u words prio=3", (unsigned)STACK_LOG);
    if (xTaskCreate(task_usb, "usb", STACK_USB, 0, 3, &g_app_ipc.task_usb) != pdPASS)
    {
        vTaskDelete(g_app_ipc.task_log);
        vTaskDelete(g_app_ipc.task_shell);
        vTaskDelete(g_app_ipc.task_wdg);
        g_app_ipc.task_log = 0;
        g_app_ipc.task_shell = 0;
        g_app_ipc.task_wdg = 0;
        return -1;
    }
    elog_info("sys", "usb stack=%u words prio=3", (unsigned)STACK_USB);
    return 0;
}

static void task_system(void *arg)
{
    (void)arg;

    bsp_log_start();
    bsp_uart_start();
    mw_log_start();

    if (bsp_init() != 0)
    {
        elog_error("boot", "bsp init failed");
    }

    if (fs_init() != 0)
    {
        elog_error("boot", "fs init failed");
    }
    if (storage_init() != 0)
    {
        elog_error("boot", "storage init failed");
    }

    if (fs_mount(fs_logfs_mount_obj()) == 0)
    {
        if (mw_log_init() != 0)
        {
            elog_warn("boot", "internal flash log disabled");
        }
    }
    else
    {
        elog_warn("boot", "internal flash log mount failed");
    }

    if (fs_mount(fs_littlefs_mount_obj()) != 0)
    {
        elog_warn("boot", "NOR littlefs mount failed");
    }

    if (mw_shell_init() != 0)
    {
        elog_error("boot", "shell init failed");
    }

    if (app_ipc_init() != 0)
    {
        elog_error("boot", "ipc init failed");
    }
    else if (create_system_tasks() != 0)
    {
        elog_error("boot", "system task create failed");
    }
    else if (app_tick_start() != 0)
    {
        elog_error("boot", "tick task create failed");
    }
    else if (app_init() != 0)
    {
        elog_error("boot", "app task create failed");
    }
    else
    {
        if (bsp_init_complete() != 0)
        {
            elog_error("boot", "bsp init complete failed");
        }
        elog_info("boot", "BSP + MW + system/app tasks ready");
    }

    vTaskDelete(NULL);
}

int system_init(void)
{
    fault_init();

    if (bsp_init_early() != 0)
    {
        return -1;
    }

    (void)elog_init();
    elog_start();

    if (g_fault_info.magic == APP_FAULT_MAGIC)
    {
        elog_error("fault", "type=%lu cfsr=%08lx hfsr=%08lx pc=%08lx lr=%08lx file=%s:%lu task=%s",
                   (unsigned long)g_fault_info.type,
                   (unsigned long)g_fault_info.cfsr,
                   (unsigned long)g_fault_info.hfsr,
                   (unsigned long)g_fault_info.pc,
                   (unsigned long)g_fault_info.lr,
                   g_fault_info.file,
                   (unsigned long)g_fault_info.line,
                   g_fault_info.task);
        g_fault_info.magic = 0U;
    }

    if (xTaskCreate(task_system, "system", STACK_SYSTEM, 0, SYSTEM_PRIO, 0) != pdPASS)
    {
        return -1;
    }
    return 0;
}
