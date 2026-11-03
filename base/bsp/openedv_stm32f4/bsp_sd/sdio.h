/**
 * @file    sdio.h
 * @brief   SD card driver over the SDIO peripheral (4-bit bus), ported from the
 *          vendor SDIO example. Used by the SD experiment and the FatFs port.
 */

#ifndef BSP_SDIO_H
#define BSP_SDIO_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

/* Pin assignment (same as the vendor board). */
#define SD_D0_GPIO_PORT    GPIOC
#define SD_D0_GPIO_PIN     GPIO_PIN_8
#define SD_D1_GPIO_PORT    GPIOC
#define SD_D1_GPIO_PIN     GPIO_PIN_9
#define SD_D2_GPIO_PORT    GPIOC
#define SD_D2_GPIO_PIN     GPIO_PIN_10
#define SD_D3_GPIO_PORT    GPIOC
#define SD_D3_GPIO_PIN     GPIO_PIN_11
#define SD_CLK_GPIO_PORT   GPIOC
#define SD_CLK_GPIO_PIN    GPIO_PIN_12
#define SD_CMD_GPIO_PORT   GPIOD
#define SD_CMD_GPIO_PIN    GPIO_PIN_2

/* Transfer clock divider: SDIO clock = 48 MHz / (div + 2) = 16 MHz. The <400 kHz
 * identification phase is handled by the SDIO init sequence, not this value. */
#define SDIO_TRANSF_CLK_DIV   1

#define SD_TIMEOUT_COUNT             ((uint32_t)100000000)
#define SD_TRANSFER_OK         ((uint8_t)0x00)
#define SD_TRANSFER_BUSY       ((uint8_t)0x01)

/* Card capacity helpers, in bytes and scaled units. */
#define SD_TOTAL_SIZE_BYTE(__Handle__)  (((uint64_t)((__Handle__)->SdCard.LogBlockNbr) * \
                                          ((__Handle__)->SdCard.LogBlockSize)) >> 0)
#define SD_TOTAL_SIZE_KB(__Handle__)    (((uint64_t)((__Handle__)->SdCard.LogBlockNbr) * \
                                          ((__Handle__)->SdCard.LogBlockSize)) >> 10)
#define SD_TOTAL_SIZE_MB(__Handle__)    (((uint64_t)((__Handle__)->SdCard.LogBlockNbr) * \
                                          ((__Handle__)->SdCard.LogBlockSize)) >> 20)
#define SD_TOTAL_SIZE_GB(__Handle__)    (((uint64_t)((__Handle__)->SdCard.LogBlockNbr) * \
                                          ((__Handle__)->SdCard.LogBlockSize)) >> 30)

extern SD_HandleTypeDef       g_sdcard_handle;
extern HAL_SD_CardInfoTypeDef g_sd_card_info_handle;

/** @brief  Card information exposed to applications (no vendor types). */
typedef struct
{
    uint32_t card_type;      /* card type: 0 = SDSC, 1 = SDHC/SDXC */
    uint32_t block_count;    /* logical block count */
    uint32_t block_size;     /* logical block size, in bytes */
    uint32_t total_size_mb;  /* card capacity, in MB */
} sd_card_info_t;

/** @brief  Initialise the SDIO peripheral and the card (4-bit bus).
 *  @return 0 on success, non-zero on failure. */
uint8_t sdio_init(void);

/** @brief  Read the cached card information. */
uint8_t get_sd_card_info(HAL_SD_CardInfoTypeDef *cardinfo);

/** @brief  Copy the cached card information into @p info. */
void sdio_get_card_info(sd_card_info_t *info);

/** @brief  Card capacity in MB, or 0 when no card is initialised. */
uint32_t sd_total_size_mb(void);

/** @brief  Return SD_TRANSFER_OK when the card is idle, SD_TRANSFER_BUSY when
 *          a transfer is still running. */
uint8_t get_sd_card_state(void);

/** @brief  Read @p cnt blocks starting at block @p saddr. */
uint8_t sd_read_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt);

/** @brief  Write @p cnt blocks starting at block @p saddr. */
uint8_t sd_write_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt);

#endif /* BSP_SDIO_H */
