#include <stdint.h>

#include "stm32f4xx_hal.h"

#define OLED_CS_PORT    GPIOA
#define OLED_CS_PIN     GPIO_PIN_4
#define OLED_DC_PORT    GPIOA
#define OLED_DC_PIN     GPIO_PIN_6
#define OLED_RST_PORT   GPIOB
#define OLED_RST_PIN    GPIO_PIN_5

#define OLED_CS_LOW()   HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_RESET)
#define OLED_CS_HIGH()  HAL_GPIO_WritePin(OLED_CS_PORT, OLED_CS_PIN, GPIO_PIN_SET)
#define OLED_DC_CMD()   HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_RESET)
#define OLED_DC_DATA()  HAL_GPIO_WritePin(OLED_DC_PORT, OLED_DC_PIN, GPIO_PIN_SET)

static SPI_HandleTypeDef g_spi1;

int oled_port_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI1;
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = 0;
    gpio.Pin = OLED_CS_PIN | OLED_DC_PIN;
    HAL_GPIO_Init(GPIOA, &gpio);
    gpio.Pin = OLED_RST_PIN;
    HAL_GPIO_Init(GPIOB, &gpio);

    OLED_CS_HIGH();
    OLED_DC_DATA();

    g_spi1.Instance               = SPI1;
    g_spi1.Init.Mode              = SPI_MODE_MASTER;
    g_spi1.Init.Direction         = SPI_DIRECTION_2LINES;
    g_spi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    g_spi1.Init.CLKPolarity       = SPI_POLARITY_LOW;
    g_spi1.Init.CLKPhase          = SPI_PHASE_1EDGE;
    g_spi1.Init.NSS               = SPI_NSS_SOFT;
    g_spi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    g_spi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    g_spi1.Init.TIMode            = SPI_TIMODE_DISABLE;
    g_spi1.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    g_spi1.Init.CRCPolynomial     = 7;
    return (HAL_SPI_Init(&g_spi1) == HAL_OK) ? 0 : -1;
}

void oled_port_reset(void)
{
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(20);
}

int oled_port_write_cmd(const uint8_t *data, uint16_t len)
{
    if ((data == 0) || (len == 0U))
    {
        return -1;
    }
    OLED_DC_CMD();
    OLED_CS_LOW();
    HAL_StatusTypeDef st = HAL_SPI_Transmit(&g_spi1, (uint8_t *)data, len, 100U);
    OLED_CS_HIGH();
    return (st == HAL_OK) ? 0 : -1;
}

int oled_port_write_data(const uint8_t *data, uint16_t len)
{
    if ((data == 0) || (len == 0U))
    {
        return -1;
    }
    OLED_DC_DATA();
    OLED_CS_LOW();
    HAL_StatusTypeDef st = HAL_SPI_Transmit(&g_spi1, (uint8_t *)data, len, 1000U);
    OLED_CS_HIGH();
    return (st == HAL_OK) ? 0 : -1;
}
