#include "FreeRTOS.h"
#include "task.h"

#include "task_shell.h"
#include "app_ipc.h"
#include "bsp_uart.h"
#include "mw_shell.h"

void task_shell(void *arg)
{
    (void)arg;

    for (;;)
    {
        uint8_t c;

        while (bsp_uart_read(BSP_UART1, &c, 1U) == 1U)
        {
            mw_shell_input((char)c);
        }

        app_wdg_kick(WDG_BIT_SHELL);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
