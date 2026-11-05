#include "stm32f4xx_hal.h"
#include "bsp_core.h"
#include "bsp_log.h"

#define BSP_PLLM 25U
#define BSP_PLLN 336U
#define BSP_PLLP 2U
#define BSP_PLLQ 7U

void HAL_MspInit(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_SYSCFG_CLK_ENABLE();
}

static HAL_StatusTypeDef board_clock_init(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    HAL_StatusTypeDef status;

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState            = RCC_HSE_ON;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM            = BSP_PLLM;
    osc.PLL.PLLN            = BSP_PLLN;
    osc.PLL.PLLP            = BSP_PLLP;
    osc.PLL.PLLQ            = BSP_PLLQ;
    status = HAL_RCC_OscConfig(&osc);
    if (status != HAL_OK)
    {
        return -1;
    }

    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                         RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4; /* PCLK1 = 42 MHz */
    clk.APB2CLKDivider = RCC_HCLK_DIV2; /* PCLK2 = 84 MHz */
    status = HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_5);
    if (status != HAL_OK)
    {
        return -1;
    }

    SCB->VTOR = FLASH_BASE;
    return HAL_OK;
}

int bsp_core_init(void)
{
    HAL_StatusTypeDef status;

    HAL_Init();
    status = board_clock_init();
    if (status != HAL_OK)
    {
        BSP_LOGE("core", "clock init failed code=%d", (int)status);
        return -1;
    }

    BSP_LOGI("core", "SYSCLK=%lu MHz HCLK=%lu MHz PCLK1=%lu MHz PCLK2=%lu MHz",
             (unsigned long)(HAL_RCC_GetSysClockFreq() / 1000000U),
             (unsigned long)(HAL_RCC_GetHCLKFreq() / 1000000U),
             (unsigned long)(HAL_RCC_GetPCLK1Freq() / 1000000U),
             (unsigned long)(HAL_RCC_GetPCLK2Freq() / 1000000U));

    /* HAL_RCC_ClockConfig already calls HAL_InitTick; call again explicitly so
     * the TIM6 timebase is configured against the final 168 MHz clock. */
    (void)HAL_InitTick(TICK_INT_PRIORITY);

#if defined(DEBUG)
    __HAL_DBGMCU_FREEZE_IWDG();
#endif

    return 0;
}


/* ---- timebase ---- */
#include "stm32f4xx_hal.h"
#include "bsp_core.h"
#include "bsp_core_clock.h"

static TIM_HandleTypeDef g_htim6;

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    uint32_t tim_clk = bsp_clock_apb1_timer_hz();
    uint32_t psc;

    if ((tim_clk < 1000000U) || ((tim_clk % 1000000U) != 0U))
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

/* Keeps this translation unit linked when bsp_core is a static archive so the
 * strong HAL_InitTick / TIM6 handlers override the HAL weak defaults. */
void bsp_core_link_anchor(void)
{
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


/* ---- watchdog ---- */
#include "stm32f4xx_hal.h"
#include "bsp_core.h"
#include "bsp_log.h"

static IWDG_HandleTypeDef g_iwdg;

void bsp_wdg_init(void)
{
    g_iwdg.Instance       = IWDG;
    g_iwdg.Init.Prescaler = IWDG_PRESCALER_128; /* LSI ~32 kHz */
    g_iwdg.Init.Reload    = 2000U;             /* ~8 s */
    (void)HAL_IWDG_Init(&g_iwdg);
    BSP_LOGI("wdg", "enabled ~8s");
}

void bsp_wdg_feed(void)
{
    (void)HAL_IWDG_Refresh(&g_iwdg);
}
