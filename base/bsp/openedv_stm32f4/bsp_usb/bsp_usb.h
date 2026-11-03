#ifndef BSP_USB_H
#define BSP_USB_H

#include "stm32f4xx_hal.h"

void bsp_usb_msp_init(PCD_HandleTypeDef *hpcd);
void bsp_usb_msp_deinit(PCD_HandleTypeDef *hpcd);

#endif /* BSP_USB_H */
