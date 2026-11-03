/**
 * @file    nor_w25q256jv.c
 * @brief   Winbond W25Q256JV (256 Mbit / 32 MB) SPI NOR chip driver: the SPI
 *          bus (SPI_BUS_NORFLASH), chip select (PF6), JEDEC probe and the
 *          address width. NOR command algorithms stay in nor.c.
 */

#include "stm32f4xx_hal.h"
#include "spi.h"
#include "nor_w25q256jv.h"

#define NOR_W25Q256JV_CS_PORT   GPIOF
#define NOR_W25Q256JV_CS_PIN    GPIO_PIN_6

#define NOR_W25Q256JV_ADDR_BYTES 4U
#define NOR_MANUFACT_DEVICE_ID   0x90U

#define NOR_CS_HIGH()  HAL_GPIO_WritePin(NOR_W25Q256JV_CS_PORT, NOR_W25Q256JV_CS_PIN, GPIO_PIN_SET)
#define NOR_CS_LOW()   HAL_GPIO_WritePin(NOR_W25Q256JV_CS_PORT, NOR_W25Q256JV_CS_PIN, GPIO_PIN_RESET)

void nor_w25q256jv_cs_low(void)
{
    NOR_CS_LOW();
}

void nor_w25q256jv_cs_high(void)
{
    NOR_CS_HIGH();
}

uint8_t nor_w25q256jv_spi_rw(uint8_t data)
{
    return spi_read_write_byte(SPI_BUS_NORFLASH, data);
}

void nor_w25q256jv_dev_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* ---- MSP begin: CS clock + pin ---- */
    __HAL_RCC_GPIOF_CLK_ENABLE();

    gpio_init.Pin   = NOR_W25Q256JV_CS_PIN;
    gpio_init.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull  = GPIO_PULLUP;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(NOR_W25Q256JV_CS_PORT, &gpio_init);
    /* ---- MSP end ---- */

    NOR_CS_HIGH();

    spi_init(SPI_BUS_NORFLASH);
    spi_set_speed(SPI_BUS_NORFLASH, SPI_BAUDRATEPRESCALER_4);
}

uint16_t nor_w25q256jv_probe(void)
{
    uint16_t deviceid;

    NOR_CS_LOW();
    (void)nor_w25q256jv_spi_rw(NOR_MANUFACT_DEVICE_ID);
    (void)nor_w25q256jv_spi_rw(0x00U);
    (void)nor_w25q256jv_spi_rw(0x00U);
    (void)nor_w25q256jv_spi_rw(0x00U);
    deviceid  = (uint16_t)(nor_w25q256jv_spi_rw(0xFFU) << 8);
    deviceid |= nor_w25q256jv_spi_rw(0xFFU);
    NOR_CS_HIGH();

    return deviceid;
}

uint8_t nor_w25q256jv_addr_bytes(void)
{
    return NOR_W25Q256JV_ADDR_BYTES;
}
