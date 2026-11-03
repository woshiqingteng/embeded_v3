/**
 * @file    spi.h
 * @brief   SPI master driver for the two on-board buses of the ALIENTEK F429
 *          board: SPI5 (NOR flash) and SPI2 (NRF24L01).
 *
 * The official example uses SPI5 on PF7/PF8/PF9 with PF6 as the flash chip
 * select, and SPI2 on PB13/PB14/PB15 for the radio module.
 */

#ifndef BSP_SPI_H
#define BSP_SPI_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

/** @brief  Selects which physical SPI bus a call applies to. */
typedef enum
{
    SPI_BUS_NORFLASH = 0, /*!< SPI5: SCK=PF7, MISO=PF8, MOSI=PF9, CS=PF6  */
    SPI_BUS_NRF24L01 = 1, /*!< SPI2: SCK=PB13, MISO=PB14, MOSI=PB15       */
} spi_bus_t;

/** @brief  Configure @p bus as an 8-bit master with the mode the device needs. */
void spi_init(spi_bus_t bus);

/** @brief  Change the baud-rate prescaler of @p bus (SPI_BAUDRATEPRESCALER_x). */
void spi_set_speed(spi_bus_t bus, uint8_t prescaler);

/** @brief  Full-duplex transfer of one byte; returns the received byte. */
uint8_t spi_read_write_byte(spi_bus_t bus, uint8_t txdata);

#endif /* BSP_SPI_H */
