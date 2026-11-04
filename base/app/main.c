#include "FreeRTOS.h"
#include "task.h"

#include "system_init.h"

int main(void)
{
    if (system_init() != 0)
    {
        for (;;)
        {
        }
    }

    vTaskStartScheduler();

    for (;;)
    {
    }
}
