#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "task_app_oled.h"
#include "app_ipc.h"
#include "app_state.h"
#include "oled.h"
#include "bsp_rtc.h"

static char g_rt_buf[512];

static void show_summary(void)
{
    app_state_t st;
    char line[24];

    app_state_get(&st);

    oled_clear();
    oled_show_string(0, 0, "STM32F429 RTOS", OLED_FONT_6X8);

    (void)snprintf(line, sizeof(line), "HEAP %lu", (unsigned long)xPortGetFreeHeapSize());
    oled_show_string(0, 8, line, OLED_FONT_6X8);

    (void)snprintf(line, sizeof(line), "TASK %lu PWM %u",
                   (unsigned long)uxTaskGetNumberOfTasks(), st.pwm_permille);
    oled_show_string(0, 16, line, OLED_FONT_6X8);

    (void)snprintf(line, sizeof(line), "ADC %u %u %u",
                   st.adc_mv[0], st.adc_mv[1], st.adc_mv[2]);
    oled_show_string(0, 24, line, OLED_FONT_6X8);

    (void)snprintf(line, sizeof(line), "ADC %u %u %u",
                   st.adc_mv[3], st.adc_mv[4], st.adc_mv[5]);
    oled_show_string(0, 32, line, OLED_FONT_6X8);

    (void)snprintf(line, sizeof(line), "KEY %lu USB %u",
                   (unsigned long)st.last_key, st.usb_connected ? 1U : 0U);
    oled_show_string(0, 40, line, OLED_FONT_6X8);

    (void)snprintf(line, sizeof(line), "TICK %lu",
                   (unsigned long)xTaskGetTickCount());
    oled_show_string(0, 48, line, OLED_FONT_6X8);

    {
        char rtc[24];
        bsp_rtc_format(rtc, sizeof(rtc));
        oled_show_string(0, 56, rtc, OLED_FONT_6X8);
    }

    oled_refresh();
}

static void show_runtime_stats(void)
{
    char line[24];
    uint8_t y = 0U;
    char *p = g_rt_buf;

    oled_clear();

    while ((*p != '\0') && (y < 64U))
    {
        uint8_t n = 0U;

        while ((*p != '\0') && (*p != '\n') && (*p != '\r') && (n < 21U))
        {
            line[n++] = *p++;
        }
        line[n] = '\0';

        while ((*p == '\n') || (*p == '\r'))
        {
            p++;
        }
        if (n > 0U)
        {
            oled_show_string(0, y, line, OLED_FONT_6X8);
            y = (uint8_t)(y + 8U);
        }
    }

    oled_refresh();
}

void task_oled(void *arg)
{
    uint32_t count = 0U;
    bool show_stats = false;

    (void)arg;

    for (;;)
    {
        (void)ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200));

        count++;
        if (count >= 5U)
        {
            count = 0U;
            show_stats = !show_stats;
            if (show_stats)
            {
                vTaskGetRunTimeStats(g_rt_buf);
            }
        }

        oled_lock();
        if (show_stats)
        {
            show_runtime_stats();
        }
        else
        {
            show_summary();
        }
        oled_unlock();

        app_wdg_kick(WDG_BIT_OLED);
    }
}
