#include "shell.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "bsp_uart.h"
#include "mw_shell.h"

static Shell             g_shell;
static char              g_shell_buf[512];
static SemaphoreHandle_t g_shell_mutex;

static short shell_write(char *data, unsigned short len)
{
    return (short)bsp_uart_write(BSP_UART1, (const uint8_t *)data, len, 100U);
}

static short shell_read(char *data, unsigned short len)
{
    return (short)bsp_uart_read(BSP_UART1, (uint8_t *)data, len);
}

static int shell_lock(Shell *shell)
{
    (void)shell;
    if (g_shell_mutex != 0)
    {
        (void)xSemaphoreTakeRecursive(g_shell_mutex, portMAX_DELAY);
    }
    return 0;
}

static int shell_unlock(Shell *shell)
{
    (void)shell;
    if (g_shell_mutex != 0)
    {
        (void)xSemaphoreGiveRecursive(g_shell_mutex);
    }
    return 0;
}

int mw_shell_init(void)
{
    g_shell_mutex = xSemaphoreCreateRecursiveMutex();

    g_shell.write  = shell_write;
    g_shell.read   = shell_read;
    g_shell.lock   = shell_lock;
    g_shell.unlock = shell_unlock;

    shellInit(&g_shell, g_shell_buf, sizeof(g_shell_buf));
    return (g_shell_mutex != 0) ? 0 : -1;
}

Shell *mw_shell_handle(void)
{
    return &g_shell;
}

void mw_shell_input(char c)
{
    shellHandler(&g_shell, c);
}
