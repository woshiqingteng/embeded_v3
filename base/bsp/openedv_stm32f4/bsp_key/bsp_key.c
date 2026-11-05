#include "stm32f4xx_hal.h"
#include "bsp_key.h"
#include "bsp_log.h"

#define BSP_KEY_SCAN_MS    10U
#define KEY_DEBOUNCE_MS    20U
#define KEY_DEBOUNCE_CNT   (KEY_DEBOUNCE_MS / BSP_KEY_SCAN_MS)

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

static key_fsm_t s_fsm[BSP_KEY_NUM];

static uint8_t key_level(bsp_key_id_t key)
{
    return (HAL_GPIO_ReadPin(s_keys[key].port, s_keys[key].pin) == s_keys[key].active)
               ? KEY_LEVEL_PRESSED
               : KEY_LEVEL_RELEASED;
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

    for (uint8_t i = 0U; i < BSP_KEY_NUM; i++)
    {
        key_fsm_init(&s_fsm[i]);
    }

    BSP_LOGI("key", "4 keys ready, debounce=%u ms", KEY_DEBOUNCE_MS);
}

uint8_t bsp_key_scan_10ms(bsp_key_event_t *events, uint8_t max_events)
{
    uint8_t count = 0U;

    if ((events == 0) || (max_events == 0U))
    {
        return 0U;
    }

    for (uint8_t i = 0U; i < BSP_KEY_NUM; i++)
    {
        key_action_t action = key_fsm_scan(&s_fsm[i], key_level((bsp_key_id_t)i),
                                           KEY_DEBOUNCE_CNT);

        if ((action != KEY_ACT_NONE) && (count < max_events))
        {
            events[count].id = (bsp_key_id_t)i;
            events[count].action = action;
            count++;
        }
    }

    return count;
}

bool bsp_key_is_pressed(bsp_key_id_t key)
{
    if (key >= BSP_KEY_NUM)
    {
        return false;
    }
    return (key_level(key) == KEY_LEVEL_PRESSED);
}
