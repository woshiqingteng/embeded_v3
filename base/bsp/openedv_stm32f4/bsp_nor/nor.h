/**
 * @file    nor.h
 * @brief   SPI NOR flash storage: device-independent read/write/erase.
 *
 * The bus, chip select and JEDEC id handling live in the chip driver
 * (nor_w25q256jv.h); this layer holds the SPI-NOR command algorithms.
 */

#ifndef BSP_NOR_H
#define BSP_NOR_H

#include <stdint.h>

#include "nor_w25q256jv.h"   /* device capacity (NOR_W25Q256JV_SIZE_BYTE) */

#define NOR_SECTOR_SIZE_BYTE    4096U
#define NOR_PAGE_SIZE_BYTE      256U

/* ---- NOR layout (single source of truth) --------------------------------
 * The chip capacity comes from the device driver; only the two partition
 * sizes are policy.  Everything else is derived so the regions can never
 * overlap or run past the end of the chip.
 *
 *   0 .. NOR_FATFS_SIZE   FatFs volume (exposed as the MSC NOR LUN)
 *   NOR_FONT_BASE ..      LVGL/GBK font store (fills up to the image store)
 *   NOR_IMAGE_BASE .. end image store (fast graphics assets)
 */
#define NOR_DEVICE_SIZE_BYTE  NOR_W25Q256JV_SIZE_BYTE
#define NOR_FATFS_SIZE_BYTE   (20U * 1024U * 1024U)   /* FatFs volume size  */
#define NOR_IMAGE_SIZE_BYTE   (128U * 1024U)          /* image store (512 KB) */

#define NOR_FONT_BASE         (NOR_FATFS_SIZE_BYTE)
#define NOR_IMAGE_BASE        (NOR_DEVICE_SIZE_BYTE - NOR_IMAGE_SIZE_BYTE)
#define NOR_FONT_SIZE_BYTE    (NOR_IMAGE_BASE - NOR_FONT_BASE)
#define NOR_FONT_SECTORS      (NOR_FONT_SIZE_BYTE / NOR_SECTOR_SIZE_BYTE)
#define NOR_IMAGE_SECTORS     (NOR_IMAGE_SIZE_BYTE / NOR_SECTOR_SIZE_BYTE)
#define NOR_FATFS_SECTORS     (NOR_FATFS_SIZE_BYTE / 512U)

#if (NOR_FONT_BASE + NOR_FONT_SIZE_BYTE) > NOR_IMAGE_BASE
#error "NOR font store overlaps the image store"
#endif
#if (NOR_IMAGE_BASE + NOR_IMAGE_SIZE_BYTE) > NOR_DEVICE_SIZE_BYTE
#error "NOR image store exceeds the chip"
#endif

/** @brief  Bring up the SPI bus and probe the device. */
void nor_init(void);

/** @brief  Read the JEDEC manufacturer/device ID. */
uint16_t nor_read_id(void);

/** @brief  Read @p datalen bytes starting at @p addr. */
void nor_read(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/** @brief  Erase-on-demand write of @p datalen bytes starting at @p addr. */
void nor_write(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/** @brief  Page-program write into an area already known to be erased (no
 *  read-back / erase); only use right after erasing the target range. */
void nor_write_erased(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/** @brief  Erase sector number @p saddr (each sector is 4 KB). */
void nor_erase_sector(uint32_t saddr);

/** @brief  Erase the 64 KB block containing @p addr (64 KB aligned); no-op if
 *  out of range. */
void nor_erase_block64(uint32_t addr);

#endif /* BSP_NOR_H */
