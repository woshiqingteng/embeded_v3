#include "FreeRTOS.h"
#include "task.h"

#include "app_tick.h"
#include "app_ipc.h"
#include "bsp_key.h"
#include "bsp_led.h"
#include "bsp_uart.h"

#define TICK_PERIOD_MS 10U

static const uint8_t s_uart2_frame[] = "USART2 TICK\r\n";

static void app_tick_run(void)
{
    static uint32_t n;

    n++;
    app_wdg_kick(WDG_BIT_TICK);

    bsp_key_id_t key = bsp_key_scan_10ms();
    if (key != BSP_KEY_NONE)
    {
        app_key_notify((uint32_t)key);
    }

    if ((n % 5U) == 0U)
    {
        if (g_app_ipc.task_adc != 0)
        {
            (void)xTaskNotifyGive(g_app_ipc.task_adc);
        }
    }

    if ((n % 10U) == 0U)
    {
        if (g_app_ipc.task_wdg != 0)
        {
            (void)xTaskNotifyGive(g_app_ipc.task_wdg);
        }
        if (g_app_ipc.task_log != 0)
        {
            (void)xTaskNotifyGive(g_app_ipc.task_log);
        }
    }

    if ((n % 20U) == 0U)
    {
        if (g_app_ipc.task_oled != 0)
        {
            (void)xTaskNotifyGive(g_app_ipc.task_oled);
        }
    }

    if ((n % 50U) == 0U)
    {
        bsp_led_toggle(BSP_LED0);
    }

    if ((n % 100U) == 0U)
    {
        (void)bsp_uart_write(BSP_UART2, s_uart2_frame,
                             sizeof(s_uart2_frame) - 1U, 0U);
        if (g_app_ipc.task_oled != 0)
        {
            (void)xTaskNotifyGive(g_app_ipc.task_oled);
        }
    }
}

static void task_tick(void *arg)
{
    TickType_t last = xTaskGetTickCount();
    (void)arg;

    for (;;)
    {
        app_tick_run();
        vTaskDelayUntil(&last, pdMS_TO_TICKS(TICK_PERIOD_MS));
    }
}

int app_tick_start(void)
{
    return (xTaskCreate(task_tick, "tick", 512U, 0, 6U,
                        &g_app_ipc.task_tick) == pdPASS) ? 0 : -1;
}
