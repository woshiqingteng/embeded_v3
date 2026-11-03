/**
 * @file    gpio_hw.h
 * @brief   Generic GPIO pin hardware attribute (port + clock + one pin + config).
 *
 * Shared by board drivers (ADC, USART, ...) that embed gpio_hw_t (one per pin)
 * to describe how a single GPIO pin is configured. Drivers assemble several
 * entries when they own several pins with possibly different configurations.
 */

#ifndef BSP_GPIO_HW_H
#define BSP_GPIO_HW_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

/** @brief GPIO hardware attributes for one pin. */
typedef struct
{
    GPIO_TypeDef *port;      /*!< GPIOA / GPIOB / ... */
    uint32_t      rcc_en;    /*!< RCC AHB1ENR clock-enable bit */
    uint16_t      pin;       /*!< the single GPIO_PIN_x */
    uint32_t      mode;      /*!< GPIO_MODE_* */
    uint32_t      pull;      /*!< GPIO_PULL* */
    uint32_t      speed;     /*!< GPIO_SPEED_FREQ_* */
    uint32_t      alternate; /*!< GPIO_AFx_* (0 for non-AF modes) */
} gpio_hw_t;

/** @brief Enable the port clock and configure the descriptor's pin. */
static inline void gpio_hw_setup(const gpio_hw_t *hw)
{
    GPIO_InitTypeDef gpio = {0};

    if (hw->pin == 0U)
    {
        return;
    }
    SET_BIT(RCC->AHB1ENR, hw->rcc_en);

    gpio.Pin       = hw->pin;
    gpio.Mode      = hw->mode;
    gpio.Pull      = hw->pull;
    gpio.Speed     = hw->speed;
    gpio.Alternate = hw->alternate;
    HAL_GPIO_Init(hw->port, &gpio);
}

#endif /* BSP_GPIO_HW_H */
