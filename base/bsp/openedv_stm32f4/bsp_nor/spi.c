/**
 * @file    spi.c
 * @brief   SPI master driver: SPI5 for the NOR flash, SPI2 for NRF24L01.
 *          MSP content (clock + GPIO) is inlined into spi_init().
 */

#include "stm32f4xx_hal.h"
#include "spi.h"

#define SPI5_GPIO_PORT      GPIOF
#define SPI5_SCK_PIN        GPIO_PIN_7
#define SPI5_MISO_PIN       GPIO_PIN_8
#define SPI5_MOSI_PIN       GPIO_PIN_9
#define SPI5_GPIO_AF        GPIO_AF5_SPI5

#define SPI2_GPIO_PORT      GPIOB
#define SPI2_SCK_PIN        GPIO_PIN_13
#define SPI2_MISO_PIN       GPIO_PIN_14
#define SPI2_MOSI_PIN       GPIO_PIN_15
#define SPI2_GPIO_AF        GPIO_AF5_SPI2

#define SPI_DUMMY_BYTE      0xFFU
#define SPI_TIMEOUT_MS      1000U

static SPI_HandleTypeDef g_spi5_handle;
static SPI_HandleTypeDef g_spi2_handle;

static SPI_HandleTypeDef *spi_handle(spi_bus_t bus)
{
    return (bus == SPI_BUS_NRF24L01) ? &g_spi2_handle : &g_spi5_handle;
}

void spi_init(spi_bus_t bus)
{
    GPIO_InitTypeDef   gpio_init = {0};
    SPI_HandleTypeDef *hspi      = spi_handle(bus);

    if (bus == SPI_BUS_NRF24L01)
    {
        /* ---- MSP begin: SPI2 clock + PB13/14/15 ---- */
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_SPI2_CLK_ENABLE();

        gpio_init.Pin       = SPI2_SCK_PIN | SPI2_MISO_PIN | SPI2_MOSI_PIN;
        gpio_init.Mode      = GPIO_MODE_AF_PP;
        gpio_init.Pull      = GPIO_PULLUP;
        gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = SPI2_GPIO_AF;
        HAL_GPIO_Init(SPI2_GPIO_PORT, &gpio_init);
        /* ---- MSP end ---- */

        hspi->Instance       = SPI2;
        hspi->Init.CLKPolarity = SPI_POLARITY_LOW;  /* mode 0 for the radio */
        hspi->Init.CLKPhase    = SPI_PHASE_1EDGE;
    }
    else
    {
        /* ---- MSP begin: SPI5 clock + PF7/8/9 ---- */
        __HAL_RCC_GPIOF_CLK_ENABLE();
        __HAL_RCC_SPI5_CLK_ENABLE();

        gpio_init.Pin       = SPI5_SCK_PIN | SPI5_MISO_PIN | SPI5_MOSI_PIN;
        gpio_init.Mode      = GPIO_MODE_AF_PP;
        gpio_init.Pull      = GPIO_PULLUP;
        gpio_init.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = SPI5_GPIO_AF;
        HAL_GPIO_Init(SPI5_GPIO_PORT, &gpio_init);
        /* ---- MSP end ---- */

        hspi->Instance       = SPI5;
        hspi->Init.CLKPolarity = SPI_POLARITY_HIGH; /* mode 3 for the flash */
        hspi->Init.CLKPhase    = SPI_PHASE_2EDGE;
    }

    hspi->Init.Mode              = SPI_MODE_MASTER;
    hspi->Init.Direction         = SPI_DIRECTION_2LINES;
    hspi->Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi->Init.NSS               = SPI_NSS_SOFT;
    /* f_SCK = f_PCLK/prescaler: SPI5 (APB2 90 MHz)/256 = 351.56 kHz,
     * SPI2 (APB1 45 MHz)/256 = 175.78 kHz. */
    hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    hspi->Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hspi->Init.TIMode            = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hspi->Init.CRCPolynomial     = 7U;
    (void)HAL_SPI_Init(hspi);

    __HAL_SPI_ENABLE(hspi);

    (void)spi_read_write_byte(bus, SPI_DUMMY_BYTE); /* flush the shift register */
}

/* Baud-rate prescaler lives in CR1[5:3]; keep every other CR1 bit. */
static void spi_baudrate_set(SPI_HandleTypeDef *hspi, uint8_t prescaler)
{
    MODIFY_REG(hspi->Instance->CR1, SPI_CR1_BR, prescaler);
}

void spi_set_speed(spi_bus_t bus, uint8_t prescaler)
{
    SPI_HandleTypeDef *hspi = spi_handle(bus);

    __HAL_SPI_DISABLE(hspi);
    spi_baudrate_set(hspi, prescaler);
    __HAL_SPI_ENABLE(hspi);
}

uint8_t spi_read_write_byte(spi_bus_t bus, uint8_t txdata)
{
    uint8_t rxdata = 0U;

    (void)HAL_SPI_TransmitReceive(spi_handle(bus), &txdata, &rxdata, 1U, SPI_TIMEOUT_MS);

    return rxdata;
}
