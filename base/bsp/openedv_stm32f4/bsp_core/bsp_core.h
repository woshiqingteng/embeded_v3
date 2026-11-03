#ifndef BSP_CORE_H
#define BSP_CORE_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

static inline uint32_t bsp_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1U;

    while (n-- != 0U)
    {
        result *= m;
    }

    return result;
}

HAL_StatusTypeDef bsp_core_init(void);
void bsp_core_delay_us(uint32_t us);
void bsp_core_delay_ms(uint32_t ms);
uint32_t bsp_core_get_tick(void);
void bsp_core_timebase_reinit(void);

void bsp_wdg_init(void);
void bsp_wdg_feed(void);

#endif /* BSP_CORE_H */
