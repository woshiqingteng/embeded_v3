/**
 * @file    nor_w25q256jv.h
 * @brief   Winbond W25Q256JV (256 Mbit / 32 MB) SPI NOR: bus, chip select,
 *          probe and addressing. Included by nor.c only.
 */

#ifndef BSP_NOR_W25Q256JV_H
#define BSP_NOR_W25Q256JV_H

#include <stdint.h>

/** @brief  Device capacity in bytes (W25Q256JV = 256 Mbit = 32 MB). */
#define NOR_W25Q256JV_SIZE_BYTE  (32U * 1024U * 1024U)

/** @brief  Bring up the SPI bus and chip select. */
void nor_w25q256jv_dev_init(void);

/** @brief  Read the JEDEC manufacturer/device ID. */
uint16_t nor_w25q256jv_probe(void);

/** @brief  Address bytes the device uses (4 for the 256 Mbit part). */
uint8_t nor_w25q256jv_addr_bytes(void);

/** @brief  Chip-select control. */
void nor_w25q256jv_cs_low(void);
void nor_w25q256jv_cs_high(void);

/** @brief  Full-duplex byte transfer on the NOR SPI bus. */
uint8_t nor_w25q256jv_spi_rw(uint8_t data);

#endif /* BSP_NOR_W25Q256JV_H */
