#include "app_ipc.h"

app_ipc_t g_app_ipc;

int app_ipc_init(void)
{
    g_app_ipc.wdg_event = xEventGroupCreate();
    g_app_ipc.state_mutex = xSemaphoreCreateMutex();

    if ((g_app_ipc.wdg_event == 0) || (g_app_ipc.state_mutex == 0))
    {
        return -1;
    }
    return 0;
}

void app_wdg_kick(uint32_t bits)
{
    if (g_app_ipc.wdg_event != 0)
    {
        (void)xEventGroupSetBits(g_app_ipc.wdg_event, bits);
    }
}

void app_key_notify(uint32_t key)
{
    if (g_app_ipc.task_ctrl != 0)
    {
        (void)xTaskNotify(g_app_ipc.task_ctrl, key, eSetValueWithOverwrite);
    }
}

void app_state_lock(void)
{
    if (g_app_ipc.state_mutex != 0)
    {
        (void)xSemaphoreTake(g_app_ipc.state_mutex, portMAX_DELAY);
    }
}

void app_state_unlock(void)
{
    if (g_app_ipc.state_mutex != 0)
    {
        (void)xSemaphoreGive(g_app_ipc.state_mutex);
    }
}
