#include "FreeRTOS.h"
#include "task.h"

void xPortSysTickHandler(void);

void SysTick_Handler(void)
{
    xPortSysTickHandler();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    __asm volatile ("cpsid i");
    for (;;)
    {
    }
}

void vApplicationMallocFailedHook(void)
{
    __asm volatile ("cpsid i");
    for (;;)
    {
    }
}
