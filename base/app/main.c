#include "FreeRTOS.h"
#include "task.h"

#include "app_init.h"

int main(void)
{
    if (app_init() != 0)
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
