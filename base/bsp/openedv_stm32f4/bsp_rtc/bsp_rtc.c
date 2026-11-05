#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_rtc_ex.h"
#include "bsp_rtc.h"
#include "bsp_log.h"

#define RTC_BACKUP_MAGIC 0x52544331U

static RTC_HandleTypeDef g_rtc;
static SemaphoreHandle_t g_rtc_lock;
static bool g_rtc_ready;

void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    (void)hrtc;
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_RTC_ENABLE();
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc)
{
    (void)hrtc;
    __HAL_RCC_RTC_DISABLE();
}

static uint8_t rtc_weekday(uint16_t y, uint8_t m, uint8_t d)
{
    static const uint8_t t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    uint16_t year = (uint16_t)(y - ((m < 3U) ? 1U : 0U));
    uint8_t w = (uint8_t)((year + year / 4U - year / 100U + year / 400U +
                           t[m - 1U] + d) % 7U);
    return (uint8_t)((w == 0U) ? RTC_WEEKDAY_SUNDAY : w);
}

static int rtc_lock(void)
{
    if (g_rtc_lock == 0)
    {
        return -1;
    }
    return (xSemaphoreTake(g_rtc_lock, portMAX_DELAY) == pdTRUE) ? 0 : -1;
}

static void rtc_unlock(void)
{
    if (g_rtc_lock != 0)
    {
        (void)xSemaphoreGive(g_rtc_lock);
    }
}

static int rtc_set_default(void)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    time.Hours = 0U;
    time.Minutes = 0U;
    time.Seconds = 0U;
    time.TimeFormat = RTC_HOURFORMAT12_AM;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;

    date.Year = 26U;
    date.Month = RTC_MONTH_JANUARY;
    date.Date = 1U;
    date.WeekDay = rtc_weekday(2026U, 1U, 1U);

    if (HAL_RTC_SetTime(&g_rtc, &time, RTC_FORMAT_BIN) != HAL_OK)
    {
        return -1;
    }
    if (HAL_RTC_SetDate(&g_rtc, &date, RTC_FORMAT_BIN) != HAL_OK)
    {
        return -1;
    }
    HAL_RTCEx_BKUPWrite(&g_rtc, RTC_BKP_DR0, RTC_BACKUP_MAGIC);
    return 0;
}

int bsp_rtc_init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_PeriphCLKInitTypeDef periph = {0};
    RTC_InitTypeDef rtc = {0};
    bool lse_ok = false;

    if (g_rtc_lock == 0)
    {
        g_rtc_lock = xSemaphoreCreateMutex();
    }
    if (g_rtc_lock == 0)
    {
        return -1;
    }

    osc.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    osc.LSEState = RCC_LSE_ON;
    if (HAL_RCC_OscConfig(&osc) == HAL_OK)
    {
        lse_ok = true;
    }
    else
    {
        osc.LSEState = RCC_LSE_OFF;
        (void)HAL_RCC_OscConfig(&osc);
        osc.OscillatorType = RCC_OSCILLATORTYPE_LSI;
        osc.LSIState = RCC_LSI_ON;
        (void)HAL_RCC_OscConfig(&osc);
    }

    periph.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    periph.RTCClockSelection = lse_ok ? RCC_RTCCLKSOURCE_LSE : RCC_RTCCLKSOURCE_LSI;
    if (HAL_RCCEx_PeriphCLKConfig(&periph) != HAL_OK)
    {
        BSP_LOGE("rtc", "periph clock config failed");
        return -1;
    }

    rtc.HourFormat = RTC_HOURFORMAT_24;
    rtc.AsynchPrediv = lse_ok ? 127U : 124U;
    rtc.SynchPrediv = 255U;
    rtc.OutPut = RTC_OUTPUT_DISABLE;
    rtc.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    rtc.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    g_rtc.Instance = RTC;
    g_rtc.Init = rtc;
    if (HAL_RTC_Init(&g_rtc) != HAL_OK)
    {
        BSP_LOGE("rtc", "init failed");
        return -1;
    }

    if (HAL_RTCEx_BKUPRead(&g_rtc, RTC_BKP_DR0) != RTC_BACKUP_MAGIC)
    {
        if (rtc_set_default() != 0)
        {
            BSP_LOGE("rtc", "set default time failed");
            return -1;
        }
    }

    g_rtc_ready = true;
    BSP_LOGI("rtc", "ready source=%s", lse_ok ? "LSE" : "LSI");
    return 0;
}

int bsp_rtc_get_datetime(bsp_rtc_datetime_t *dt)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};
    int ret = -1;

    if ((dt == 0) || !g_rtc_ready || (rtc_lock() != 0))
    {
        return -1;
    }

    if ((HAL_RTC_GetTime(&g_rtc, &time, RTC_FORMAT_BIN) == HAL_OK) &&
        (HAL_RTC_GetDate(&g_rtc, &date, RTC_FORMAT_BIN) == HAL_OK))
    {
        dt->year = (uint16_t)(2000U + date.Year);
        dt->month = date.Month;
        dt->date = date.Date;
        dt->hour = time.Hours;
        dt->minute = time.Minutes;
        dt->second = time.Seconds;
        dt->weekday = date.WeekDay;
        ret = 0;
    }

    rtc_unlock();
    return ret;
}

int bsp_rtc_set_datetime(const bsp_rtc_datetime_t *dt)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};
    int ret = -1;

    if ((dt == 0) || !g_rtc_ready || (rtc_lock() != 0))
    {
        return -1;
    }

    time.Hours = dt->hour;
    time.Minutes = dt->minute;
    time.Seconds = dt->second;
    time.TimeFormat = RTC_HOURFORMAT12_AM;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;

    date.Year = (uint8_t)(dt->year - 2000U);
    date.Month = dt->month;
    date.Date = dt->date;
    date.WeekDay = rtc_weekday(dt->year, dt->month, dt->date);

    if ((HAL_RTC_SetTime(&g_rtc, &time, RTC_FORMAT_BIN) == HAL_OK) &&
        (HAL_RTC_SetDate(&g_rtc, &date, RTC_FORMAT_BIN) == HAL_OK))
    {
        HAL_RTCEx_BKUPWrite(&g_rtc, RTC_BKP_DR0, RTC_BACKUP_MAGIC);
        ret = 0;
    }

    rtc_unlock();
    return ret;
}

void bsp_rtc_format(char *buf, size_t size)
{
    bsp_rtc_datetime_t dt;

    if ((buf == 0) || (size == 0U))
    {
        return;
    }
    if (bsp_rtc_get_datetime(&dt) == 0)
    {
        (void)snprintf(buf, size, "%04u-%02u-%02u %02u:%02u:%02u",
                       (unsigned)dt.year, (unsigned)dt.month, (unsigned)dt.date,
                       (unsigned)dt.hour, (unsigned)dt.minute, (unsigned)dt.second);
    }
    else
    {
        (void)snprintf(buf, size, "RTC --");
    }
}
