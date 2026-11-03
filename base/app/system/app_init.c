#include "FreeRTOS.h"
#include "task.h"

#include "app_init.h"
#include "app_ipc.h"
#include "app_state.h"
#include "app_tick.h"

#include "elog.h"
#include "mw_littlefs.h"
#include "mw_log.h"
#include "mw_shell.h"
#include "mw_usb_bridge.h"

#include "bsp_core.h"
#include "bsp_log.h"
#include "bsp_uart.h"
#include "bsp_led.h"
#include "bsp_key.h"
#include "bsp_pwm.h"
#include "bsp_adc.h"
#include "oled.h"
#include "nor.h"
#include "bsp_sd.h"

#include "task_ctrl.h"
#include "task_adc.h"
#include "task_wdg.h"
#include "task_log.h"
#include "task_oled.h"
#include "task_shell.h"
#include "task_usb.h"

#define STACK_WDG   256U
#define STACK_CTRL  256U
#define STACK_ADC   256U
#define STACK_SHELL 768U
#define STACK_LOG   1024U
#define STACK_USB   768U
#define STACK_OLED  1024U

static void irq_unmask(void)
{
    __set_BASEPRI(0U);
    __enable_irq();
}

static int create_tasks(void)
{
    if (xTaskCreate(task_wdg, "wdg", STACK_WDG, 0, 8, &g_app_ipc.task_wdg) != pdPASS) return -1;
    if (xTaskCreate(task_ctrl, "ctrl", STACK_CTRL, 0, 5, &g_app_ipc.task_ctrl) != pdPASS) return -1;
    if (xTaskCreate(task_adc, "adc", STACK_ADC, 0, 4, &g_app_ipc.task_adc) != pdPASS) return -1;
    if (xTaskCreate(task_shell, "shell", STACK_SHELL, 0, 4, &g_app_ipc.task_shell) != pdPASS) return -1;
    if (xTaskCreate(task_log, "log", STACK_LOG, 0, 3, &g_app_ipc.task_log) != pdPASS) return -1;
    if (xTaskCreate(task_usb, "usb", STACK_USB, 0, 3, &g_app_ipc.task_usb) != pdPASS) return -1;
    if (xTaskCreate(task_oled, "oled", STACK_OLED, 0, 2, &g_app_ipc.task_oled) != pdPASS) return -1;
    return 0;
}

int app_init(void)
{
    if (bsp_core_init() != HAL_OK)
    {
        return -1;
    }

    bsp_log_init();
    (void)elog_init();
    elog_start();

    bsp_uart_init();
    bsp_log_attach_uart();
    irq_unmask();

    bsp_led_init();
    bsp_key_init();
    bsp_pwm_init();
    bsp_adc_init();

    irq_unmask();
    nor_init();
    irq_unmask();
    if (mw_littlefs_init() == 0 && mw_littlefs_mount() == 0)
    {
        (void)mw_log_init();
    }
    else
    {
        elog_warn("boot", "littlefs not mounted; file log disabled");
    }
    irq_unmask();
    (void)bsp_sd_init();
    irq_unmask();
    oled_init();
    (void)mw_shell_init();
    irq_unmask();

    if (app_ipc_init() != 0)
    {
        return -1;
    }
    app_state_init();
    irq_unmask();

    if (create_tasks() != 0)
    {
        return -1;
    }

    if (app_tick_start() != 0)
    {
        return -1;
    }
    irq_unmask();

    bsp_wdg_init();
    elog_info("boot", "BSP + RTOS init complete");

    return 0;
}
