#include "stm32f4xx_hal.h"
#include "bsp_core.h"

static TIM_HandleTypeDef g_htim6;

static uint32_t tim6_input_clock(void)
{
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();
    uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1);

    /* APB1 prescaler != 1 -> timer clock is twice PCLK1. */
    return (ppre1 == RCC_CFGR_PPRE1_DIV1) ? pclk1 : (pclk1 * 2U);
}

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    uint32_t tim_clk = tim6_input_clock();
    uint32_t psc;

    if (tim_clk < 1000000U)
    {
        return HAL_ERROR;
    }

    psc = (tim_clk / 1000000U) - 1U;

    __HAL_RCC_TIM6_CLK_ENABLE();

    if (g_htim6.Instance != 0)
    {
        (void)HAL_TIM_Base_Stop_IT(&g_htim6);
    }

    g_htim6.Instance               = TIM6;
    g_htim6.Init.Prescaler         = psc;
    g_htim6.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_htim6.Init.Period            = 999U; /* 1 MHz / 1000 = 1 kHz */
    g_htim6.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    if (HAL_TIM_Base_Init(&g_htim6) != HAL_OK)
    {
        return HAL_ERROR;
    }

    uwTickPrio = TickPriority;
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, TickPriority, 0U);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);

    if (HAL_TIM_Base_Start_IT(&g_htim6) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

void HAL_SuspendTick(void)
{
    __HAL_TIM_DISABLE_IT(&g_htim6, TIM_IT_UPDATE);
}

void HAL_ResumeTick(void)
{
    __HAL_TIM_ENABLE_IT(&g_htim6, TIM_IT_UPDATE);
}

void bsp_core_timebase_reinit(void)
{
    (void)HAL_InitTick(uwTickPrio);
}

void TIM6_DAC_IRQHandler(void)
{
    if ((__HAL_TIM_GET_FLAG(&g_htim6, TIM_FLAG_UPDATE) != RESET) &&
        (__HAL_TIM_GET_IT_SOURCE(&g_htim6, TIM_IT_UPDATE) != RESET))
    {
        __HAL_TIM_CLEAR_IT(&g_htim6, TIM_IT_UPDATE);
        HAL_IncTick();
    }
}

void bsp_core_delay_us(uint32_t us)
{
    uint32_t start = __HAL_TIM_GET_COUNTER(&g_htim6);

    while ((uint32_t)(__HAL_TIM_GET_COUNTER(&g_htim6) - start) < us)
    {
    }
}
