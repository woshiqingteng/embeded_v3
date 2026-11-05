#include "stm32f4xx_hal.h"
#include "bsp_core_clock.h"

static TIM_HandleTypeDef g_tim7;
static volatile uint32_t g_runtime_ticks;

void bsp_runtime_stats_init(void)
{
    uint32_t tim_clk = bsp_clock_apb1_timer_hz();
    uint32_t psc;

    if ((tim_clk < 1000000U) || ((tim_clk % 1000000U) != 0U))
    {
        return;
    }
    psc = (tim_clk / 1000000U) - 1U;

    __HAL_RCC_TIM7_CLK_ENABLE();

    g_tim7.Instance               = TIM7;
    g_tim7.Init.Prescaler         = psc;  /* timer clock / 1 MHz */
    g_tim7.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_tim7.Init.Period            = 99U;  /* 1 MHz / 100 = 10 kHz */
    g_tim7.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_tim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    (void)HAL_TIM_Base_Init(&g_tim7);

    HAL_NVIC_SetPriority(TIM7_IRQn, 15U, 0U);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);
    (void)HAL_TIM_Base_Start_IT(&g_tim7);
}

uint32_t bsp_runtime_stats_get(void)
{
    return g_runtime_ticks;
}

void TIM7_IRQHandler(void)
{
    if ((__HAL_TIM_GET_FLAG(&g_tim7, TIM_FLAG_UPDATE) != RESET) &&
        (__HAL_TIM_GET_IT_SOURCE(&g_tim7, TIM_IT_UPDATE) != RESET))
    {
        __HAL_TIM_CLEAR_IT(&g_tim7, TIM_IT_UPDATE);
        g_runtime_ticks++;
    }
}
