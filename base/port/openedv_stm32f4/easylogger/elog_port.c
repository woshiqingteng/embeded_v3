#include <stdio.h>

#include "elog.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "stm32f4xx_hal.h"
#include "bsp_log.h"

static SemaphoreHandle_t g_elog_lock;

ElogErrCode elog_port_init(void)
{
    g_elog_lock = xSemaphoreCreateRecursiveMutex();
    return ELOG_NO_ERR;
}

void elog_port_deinit(void)
{
}

void elog_port_output(const char *log, size_t size)
{
    bsp_log_write((const uint8_t *)log, (uint32_t)size);
}

void elog_port_output_lock(void)
{
    if (g_elog_lock != 0)
    {
        TickType_t ticks = (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
                               ? 0U : portMAX_DELAY;
        (void)xSemaphoreTakeRecursive(g_elog_lock, ticks);
    }
}

void elog_port_output_unlock(void)
{
    if (g_elog_lock != 0)
    {
        (void)xSemaphoreGiveRecursive(g_elog_lock);
    }
}

const char *elog_port_get_time(void)
{
    static char time_buf[16];
    uint32_t ms = HAL_GetTick();

    (void)snprintf(time_buf, sizeof(time_buf), "%lu.%03lu",
                   (unsigned long)(ms / 1000U), (unsigned long)(ms % 1000U));
    return time_buf;
}

const char *elog_port_get_p_info(void)
{
    return "";
}

const char *elog_port_get_t_info(void)
{
    return "";
}
