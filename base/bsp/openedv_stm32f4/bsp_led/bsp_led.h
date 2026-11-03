#ifndef BSP_LED_H
#define BSP_LED_H

#include <stdbool.h>

typedef enum
{
    BSP_LED0 = 0,
    BSP_LED1,
    BSP_LED_NUM
} bsp_led_t;

void bsp_led_init(void);
void bsp_led_on(bsp_led_t led);
void bsp_led_off(bsp_led_t led);
void bsp_led_toggle(bsp_led_t led);
bool bsp_led_is_on(bsp_led_t led);

#endif /* BSP_LED_H */
