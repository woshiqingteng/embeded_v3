#include "stm32f4xx_hal.h"
#include "bsp_led.h"
#include "bsp_log.h"

#define LED0_PORT GPIOB
#define LED0_PIN  GPIO_PIN_1
#define LED1_PORT GPIOB
#define LED1_PIN  GPIO_PIN_0

void bsp_led_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Pin   = LED0_PIN | LED1_PIN;
    HAL_GPIO_Init(GPIOB, &gpio);

    bsp_led_off(BSP_LED0);
    bsp_led_off(BSP_LED1);
    BSP_LOGI("led", "2 channels ready");
}

void bsp_led_on(bsp_led_t led)
{
    if (led == BSP_LED0)
    {
        HAL_GPIO_WritePin(LED0_PORT, LED0_PIN, GPIO_PIN_RESET);
    }
    else if (led == BSP_LED1)
    {
        HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);
    }
}

void bsp_led_off(bsp_led_t led)
{
    if (led == BSP_LED0)
    {
        HAL_GPIO_WritePin(LED0_PORT, LED0_PIN, GPIO_PIN_SET);
    }
    else if (led == BSP_LED1)
    {
        HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);
    }
}

void bsp_led_toggle(bsp_led_t led)
{
    if (led == BSP_LED0)
    {
        HAL_GPIO_TogglePin(LED0_PORT, LED0_PIN);
    }
    else if (led == BSP_LED1)
    {
        HAL_GPIO_TogglePin(LED1_PORT, LED1_PIN);
    }
}

bool bsp_led_is_on(bsp_led_t led)
{
    if (led == BSP_LED0)
    {
        return HAL_GPIO_ReadPin(LED0_PORT, LED0_PIN) == GPIO_PIN_RESET;
    }
    if (led == BSP_LED1)
    {
        return HAL_GPIO_ReadPin(LED1_PORT, LED1_PIN) == GPIO_PIN_RESET;
    }
    return false;
}
