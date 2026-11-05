/**
 * @file    nor.h
 * @brief   SPI NOR flash storage: device-independent read/write/erase.
 *
 * The bus, chip select, JEDEC id and command algorithms are contained in
 * nor.c; this header exposes only the storage API.
 */

#ifndef BSP_NOR_H
#define BSP_NOR_H

#include <stdint.h>


#define NOR_SECTOR_SIZE_BYTE    4096U
#define NOR_PAGE_SIZE_BYTE      256U
#define NOR_DEVICE_SIZE_BYTE    (32U * 1024U * 1024U)

/** @brief  Bring up the SPI bus and probe the device. */
void nor_init(void);

/** @brief  Read the JEDEC manufacturer/device ID. */
uint16_t nor_read_id(void);

/** @brief  Read @p datalen bytes starting at @p addr. */
int nor_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/** @brief  Erase-on-demand write of @p datalen bytes starting at @p addr. */
int nor_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/** @brief  Page-program write into an area already known to be erased (no
 *  read-back / erase); only use right after erasing the target range. */
int nor_write_erased(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/** @brief  Erase sector number @p saddr (each sector is 4 KB). */
int nor_erase_sector(uint32_t saddr);

/** @brief  Erase the 64 KB block containing @p addr (64 KB aligned); no-op if
 *  out of range. */
int nor_erase_block64(uint32_t addr);

#endif /* BSP_NOR_H */
