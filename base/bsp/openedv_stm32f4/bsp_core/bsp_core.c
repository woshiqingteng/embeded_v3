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
        return status;
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
        return status;
    }

    SCB->VTOR = FLASH_BASE;
    return HAL_OK;
}

HAL_StatusTypeDef bsp_core_init(void)
{
    HAL_StatusTypeDef status;

    HAL_Init();
    status = board_clock_init();
    if (status != HAL_OK)
    {
        BSP_LOGE("core", "clock init failed code=%d", (int)status);
        return status;
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

    return HAL_OK;
}

uint32_t bsp_core_get_tick(void)
{
    return HAL_GetTick();
}

void bsp_core_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}
