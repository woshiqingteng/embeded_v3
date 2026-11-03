#include "stm32f4xx_hal.h"
#include "bsp_key.h"

#define KEY0_PORT   GPIOH
#define KEY0_PIN    GPIO_PIN_3
#define KEY1_PORT   GPIOH
#define KEY1_PIN    GPIO_PIN_2
#define KEY2_PORT   GPIOC
#define KEY2_PIN    GPIO_PIN_13
#define WKUP_PORT   GPIOA
#define WKUP_PIN    GPIO_PIN_0

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      pin;
    GPIO_PinState active;
} key_hw_t;

static const key_hw_t s_keys[BSP_KEY_NUM] =
{
    { KEY0_PORT, KEY0_PIN, GPIO_PIN_RESET },
    { KEY1_PORT, KEY1_PIN, GPIO_PIN_RESET },
    { KEY2_PORT, KEY2_PIN, GPIO_PIN_RESET },
    { WKUP_PORT, WKUP_PIN, GPIO_PIN_SET },
};

static uint8_t s_stable[BSP_KEY_NUM];
static uint8_t s_candidate[BSP_KEY_NUM];
static uint8_t s_count[BSP_KEY_NUM];

static uint8_t key_raw(bsp_key_id_t key)
{
    return (HAL_GPIO_ReadPin(s_keys[key].port, s_keys[key].pin) == s_keys[key].active) ? 1U : 0U;
}

void bsp_key_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Pin  = KEY0_PIN | KEY1_PIN;
    HAL_GPIO_Init(GPIOH, &gpio);

    gpio.Pin = KEY2_PIN;
    HAL_GPIO_Init(GPIOC, &gpio);

    gpio.Pull = GPIO_PULLDOWN;
    gpio.Pin  = WKUP_PIN;
    HAL_GPIO_Init(GPIOA, &gpio);
}

bsp_key_id_t bsp_key_scan_10ms(void)
{
    for (uint8_t i = 0U; i < BSP_KEY_NUM; i++)
    {
        uint8_t raw = key_raw((bsp_key_id_t)i);

        if (raw == s_candidate[i])
        {
            if (s_count[i] < 3U)
            {
                s_count[i]++;
            }
        }
        else
        {
            s_candidate[i] = raw;
            s_count[i] = 0U;
        }

        if ((s_count[i] >= 2U) && (s_stable[i] != s_candidate[i]))
        {
            s_stable[i] = s_candidate[i];
            s_count[i] = 0U;
            if (s_stable[i] != 0U)
            {
                return (bsp_key_id_t)i;
            }
        }
    }

    return BSP_KEY_NONE;
}

bool bsp_key_is_pressed(bsp_key_id_t key)
{
    if (key >= BSP_KEY_NUM)
    {
        return false;
    }
    return key_raw(key) != 0U;
}
