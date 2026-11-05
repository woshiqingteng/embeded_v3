#ifndef BSP_RTC_H
#define BSP_RTC_H

#include <stdint.h>
#include <stddef.h>

typedef struct
{
    uint16_t year;
    uint8_t  month;
    uint8_t  date;
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  second;
    uint8_t  weekday;
} bsp_rtc_datetime_t;

int  bsp_rtc_init(void);
int  bsp_rtc_get_datetime(bsp_rtc_datetime_t *dt);
int  bsp_rtc_set_datetime(const bsp_rtc_datetime_t *dt);
void bsp_rtc_format(char *buf, size_t size);

#endif /* BSP_RTC_H */
