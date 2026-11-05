#include "FreeRTOS.h"
#include "task.h"

#include "fault_info.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    fault_stack_overflow(pcTaskName);
}

void vApplicationMallocFailedHook(void)
{
    fault_record_reset(APP_FAULT_TYPE_MALLOC);
}
