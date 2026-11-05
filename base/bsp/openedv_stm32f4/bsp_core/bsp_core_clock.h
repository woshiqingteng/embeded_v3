#ifndef BSP_CORE_CLOCK_H
#define BSP_CORE_CLOCK_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

static inline uint32_t bsp_clock_pclk1(void)
{
    return HAL_RCC_GetPCLK1Freq();
}

static inline uint32_t bsp_clock_pclk2(void)
{
    return HAL_RCC_GetPCLK2Freq();
}

static inline uint32_t bsp_clock_apb1_timer_hz(void)
{
    uint32_t pclk = HAL_RCC_GetPCLK1Freq();
    uint32_t ppre = (RCC->CFGR & RCC_CFGR_PPRE1);

    return (ppre == RCC_CFGR_PPRE1_DIV1) ? pclk : (pclk * 2U);
}

static inline uint32_t bsp_clock_apb2_timer_hz(void)
{
    uint32_t pclk = HAL_RCC_GetPCLK2Freq();
    uint32_t ppre = (RCC->CFGR & RCC_CFGR_PPRE2);

    return (ppre == RCC_CFGR_PPRE2_DIV1) ? pclk : (pclk * 2U);
}

static inline uint32_t bsp_clock_psc_for(uint32_t timer_hz, uint32_t target_hz)
{
    if ((timer_hz < target_hz) || (target_hz == 0U) ||
        ((timer_hz % target_hz) != 0U))
    {
        return 0U;
    }
    return (timer_hz / target_hz) - 1U;
}

#endif /* BSP_CORE_CLOCK_H */
