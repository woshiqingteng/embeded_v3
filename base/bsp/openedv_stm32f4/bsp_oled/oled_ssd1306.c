#include <string.h>

#include "stm32f4xx_hal.h"
#include "oled_ssd1306.h"

/* SPI1: SCK=PA5, MOSI=PA7; CS=PA4, DC=PA6, RST=PB5. */
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

static void oled_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF5_SPI1;
    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_7; /* SCK + MOSI */
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
}

static void oled_spi_init(void)
{
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
    (void)HAL_SPI_Init(&g_spi1);
}

static void oled_write_byte(uint8_t data, uint8_t is_data)
{
    if (is_data)
    {
        OLED_DC_DATA();
    }
    else
    {
        OLED_DC_CMD();
    }

    OLED_CS_LOW();
    (void)HAL_SPI_Transmit(&g_spi1, &data, 1U, 100U);
    OLED_CS_HIGH();
}

static void oled_write_data(const uint8_t *data, uint16_t len)
{
    OLED_DC_DATA();
    OLED_CS_LOW();
    (void)HAL_SPI_Transmit(&g_spi1, (uint8_t *)data, len, 1000U);
    OLED_CS_HIGH();
}

static void oled_reset(void)
{
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(OLED_RST_PORT, OLED_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(20);
}

void oled_ssd1306_init(void)
{
    static const uint8_t init_seq[] = {
        0xAE,
        0xD5, 0x50,
        0xA8, 0x3F,
        0xD3, 0x00,
        0x40,
        0x8D, 0x14,
        0x20, 0x02,
        0xA1,
        0xC8,
        0xDA, 0x12,
        0x81, 0xEF,
        0xD9, 0xF1,
        0xDB, 0x30,
        0xA4,
        0xA6,
        0xAF,
    };

    oled_gpio_init();
    oled_spi_init();
    oled_reset();

    for (uint16_t i = 0U; i < sizeof(init_seq); i++)
    {
        oled_write_byte(init_seq[i], 0U);
    }
}

void oled_ssd1306_display_on(void)
{
    oled_write_byte(0x8D, 0U);
    oled_write_byte(0x14, 0U);
    oled_write_byte(0xAF, 0U);
}

void oled_ssd1306_display_off(void)
{
    oled_write_byte(0x8D, 0U);
    oled_write_byte(0x10, 0U);
    oled_write_byte(0xAE, 0U);
}

void oled_ssd1306_flush(const uint8_t *gram, uint8_t width, uint8_t pages)
{
    uint8_t line[128];

    for (uint8_t page = 0U; page < pages; page++)
    {
        oled_write_byte((uint8_t)(0xB0U + page), 0U);
        oled_write_byte(0x00, 0U);
        oled_write_byte(0x10, 0U);

        for (uint8_t col = 0U; col < width; col++)
        {
            line[col] = gram[(uint32_t)col * pages + page];
        }

        oled_write_data(line, width);
    }
}
