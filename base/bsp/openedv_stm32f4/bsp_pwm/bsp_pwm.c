#include "stm32f4xx_hal.h"
#include "bsp_pwm.h"
#include "bsp_core_clock.h"
#include "bsp_log.h"

#define PWM_FREQ_HZ      1000U
#define PWM_MAX_DUTY     1000U

static TIM_HandleTypeDef g_htim8;
static uint16_t g_duty = 500U;

static void pwm_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio.Pin       = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init(GPIOC, &gpio);
}

void bsp_pwm_init(void)
{
    TIM_OC_InitTypeDef oc = {0};
    uint32_t tim_clk = bsp_clock_apb2_timer_hz();
    uint32_t psc;
    uint32_t period;

    if ((tim_clk < 1000000U) || ((tim_clk % 1000000U) != 0U))
    {
        BSP_LOGE("pwm", "unsupported TIM8 clock %lu", (unsigned long)tim_clk);
        return;
    }
    psc = (tim_clk / 1000000U) - 1U;
    period = (1000000U / PWM_FREQ_HZ) - 1U;

    __HAL_RCC_TIM8_CLK_ENABLE();
    pwm_gpio_init();

    g_htim8.Instance               = TIM8;
    g_htim8.Init.Prescaler         = psc;
    g_htim8.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_htim8.Init.Period            = period;
    g_htim8.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_htim8.Init.RepetitionCounter = 0U;
    g_htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    (void)HAL_TIM_PWM_Init(&g_htim8);

    oc.OCMode       = TIM_OCMODE_PWM1;
    oc.Pulse        = g_duty;
    oc.OCPolarity   = TIM_OCPOLARITY_HIGH;
    oc.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    oc.OCFastMode   = TIM_OCFAST_DISABLE;
    oc.OCIdleState  = TIM_OCIDLESTATE_RESET;
    oc.OCNIdleState = TIM_OCNIDLESTATE_RESET;

    (void)HAL_TIM_PWM_ConfigChannel(&g_htim8, &oc, TIM_CHANNEL_1);
    (void)HAL_TIM_PWM_ConfigChannel(&g_htim8, &oc, TIM_CHANNEL_2);

    (void)HAL_TIM_PWM_Start(&g_htim8, TIM_CHANNEL_1);
    (void)HAL_TIM_PWM_Start(&g_htim8, TIM_CHANNEL_2);
    BSP_LOGI("pwm", "TIM8 duty=%u/1000 permille", (unsigned)g_duty);
}

void bsp_pwm_set_duty(uint16_t permille)
{
    if (permille > PWM_MAX_DUTY)
    {
        permille = PWM_MAX_DUTY;
    }

    g_duty = permille;
    __HAL_TIM_SET_COMPARE(&g_htim8, TIM_CHANNEL_1, g_duty);
    __HAL_TIM_SET_COMPARE(&g_htim8, TIM_CHANNEL_2, g_duty);
}

uint16_t bsp_pwm_get_duty(void)
{
    return g_duty;
}
