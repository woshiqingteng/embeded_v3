#ifndef BSP_INTERNAL_FLASH_H
#define BSP_INTERNAL_FLASH_H

#include <stdint.h>

/* Last 256 KiB of the 1 MiB internal flash: sectors 10 and 11. */
#define BSP_INT_FLASH_BASE          0x080C0000U
#define BSP_INT_FLASH_SIZE          (256U * 1024U)
#define BSP_INT_FLASH_SECTOR_FIRST  10U
#define BSP_INT_FLASH_SECTOR_COUNT  2U
#define BSP_INT_FLASH_SECTOR_SIZE   (128U * 1024U)

int  bsp_internal_flash_init(void);
int  bsp_internal_flash_read(uint32_t addr, void *buf, uint32_t len);
int  bsp_internal_flash_prog(uint32_t addr, const void *buf, uint32_t len);
int  bsp_internal_flash_erase_range(uint32_t addr, uint32_t len);

#endif /* BSP_INTERNAL_FLASH_H */
