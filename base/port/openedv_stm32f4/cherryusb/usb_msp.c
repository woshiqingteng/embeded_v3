#include "stm32f4xx_hal.h"

#define USB_MSP_IRQ_PRIORITY 5U

void bsp_usb_msp_init(PCD_HandleTypeDef *hpcd)
{
    GPIO_InitTypeDef gpio = {0};

    if ((hpcd == 0) || (hpcd->Instance != USB_OTG_FS))
    {
        return;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin       = GPIO_PIN_11 | GPIO_PIN_12;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF10_OTG_FS;
    HAL_GPIO_Init(GPIOA, &gpio);

    __HAL_RCC_USB_OTG_FS_CLK_ENABLE();

    HAL_NVIC_SetPriority(OTG_FS_IRQn, USB_MSP_IRQ_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
}

void bsp_usb_msp_deinit(PCD_HandleTypeDef *hpcd)
{
    if ((hpcd == 0) || (hpcd->Instance != USB_OTG_FS))
    {
        return;
    }

    __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
    HAL_NVIC_DisableIRQ(OTG_FS_IRQn);
}

/* Strong overrides of the HAL weak MSP hooks called by CherryUSB's DWC2 glue. */
void HAL_PCD_MspInit(PCD_HandleTypeDef *hpcd)
{
    bsp_usb_msp_init(hpcd);
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef *hpcd)
{
    bsp_usb_msp_deinit(hpcd);
}
