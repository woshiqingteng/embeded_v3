#ifndef BSP_PWM_H
#define BSP_PWM_H

#include <stdint.h>

void bsp_pwm_init(void);
void bsp_pwm_set_duty(uint16_t permille);
uint16_t bsp_pwm_get_duty(void);

#endif /* BSP_PWM_H */
