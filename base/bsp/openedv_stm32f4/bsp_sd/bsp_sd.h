#ifndef BSP_SD_H
#define BSP_SD_H

#include <stdbool.h>
#include <stdint.h>

#include "sdio.h"

typedef enum
{
    BSP_SD_OWNER_NONE = 0,
    BSP_SD_OWNER_FATFS,
    BSP_SD_OWNER_MSC
} bsp_sd_owner_t;

typedef void (*bsp_sd_io_hook_t)(void);

/* The current board SD card is write-protected/fails all write attempts.
 * Keep the default read-only; set to 1 only after the card is validated. */
#ifndef BSP_SD_WRITABLE
#define BSP_SD_WRITABLE 0
#endif

int  bsp_sd_init(void);
int  bsp_sd_get_info(sd_card_info_t *info);

int  bsp_sd_acquire(bsp_sd_owner_t owner, uint32_t timeout_ms);
void bsp_sd_release(bsp_sd_owner_t owner);
bsp_sd_owner_t bsp_sd_owner(void);
bool bsp_sd_is_writable(void);
int  bsp_sd_set_dma_enabled(bool enable);
bool bsp_sd_dma_enabled(void);

void bsp_sd_set_io_hook(bsp_sd_io_hook_t hook);

int bsp_sd_read(uint8_t *buf, uint32_t lba, uint32_t count);
int bsp_sd_write(const uint8_t *buf, uint32_t lba, uint32_t count);

#endif /* BSP_SD_H */
