#ifndef APP_IPC_H
#define APP_IPC_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "semphr.h"

#define WDG_BIT_TICK    (1U << 0)
#define WDG_BIT_CTRL    (1U << 1)
#define WDG_BIT_ADC     (1U << 2)
#define WDG_BIT_LOG     (1U << 3)
#define WDG_BIT_OLED    (1U << 4)
#define WDG_BIT_SHELL   (1U << 5)
#define WDG_BIT_USB     (1U << 6)
#define WDG_BIT_ALL     (WDG_BIT_TICK | WDG_BIT_CTRL | WDG_BIT_ADC | \
                         WDG_BIT_LOG | WDG_BIT_OLED | WDG_BIT_SHELL | WDG_BIT_USB)

typedef struct
{
    TaskHandle_t task_ctrl;
    TaskHandle_t task_adc;
    TaskHandle_t task_wdg;
    TaskHandle_t task_log;
    TaskHandle_t task_oled;
    TaskHandle_t task_shell;
    TaskHandle_t task_usb;
    TaskHandle_t task_tick;

    EventGroupHandle_t wdg_event;
    SemaphoreHandle_t  state_mutex;
} app_ipc_t;

extern app_ipc_t g_app_ipc;

int  app_ipc_init(void);
void app_wdg_kick(uint32_t bits);
void app_key_notify(uint32_t key);
void app_state_lock(void);
void app_state_unlock(void);

#endif /* APP_IPC_H */
