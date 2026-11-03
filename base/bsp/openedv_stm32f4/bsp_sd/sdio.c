/**
 * @file    sdio.c
 * @brief   SD card driver over SDIO, ported from the vendor SDIO example.
 *          MSP content (clock + GPIO) is inlined into sdio_init().
 */

#include "string.h"
#include "sdio.h"

SD_HandleTypeDef       g_sdcard_handle;
HAL_SD_CardInfoTypeDef g_sd_card_info_handle;

uint8_t sdio_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /* ---- MSP begin: SDIO clock + PC8..PC12/PD2 ---- */
    __HAL_RCC_SDIO_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    gpio_init.Mode      = GPIO_MODE_AF_PP;
    gpio_init.Pull      = GPIO_PULLUP;
    gpio_init.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio_init.Alternate = GPIO_AF12_SDIO;

    gpio_init.Pin = SD_D0_GPIO_PIN;
    HAL_GPIO_Init(SD_D0_GPIO_PORT, &gpio_init);
    gpio_init.Pin = SD_D1_GPIO_PIN;
    HAL_GPIO_Init(SD_D1_GPIO_PORT, &gpio_init);
    gpio_init.Pin = SD_D2_GPIO_PIN;
    HAL_GPIO_Init(SD_D2_GPIO_PORT, &gpio_init);
    gpio_init.Pin = SD_D3_GPIO_PIN;
    HAL_GPIO_Init(SD_D3_GPIO_PORT, &gpio_init);
    gpio_init.Pin = SD_CLK_GPIO_PIN;
    HAL_GPIO_Init(SD_CLK_GPIO_PORT, &gpio_init);
    gpio_init.Pin = SD_CMD_GPIO_PIN;
    HAL_GPIO_Init(SD_CMD_GPIO_PORT, &gpio_init);
    /* ---- MSP end ---- */

    /* Initialisation must stay below 400 kHz, hence the bypass disabled and a
     * wide divider during the identification phase. */
    g_sdcard_handle.Instance            = SDIO;
    g_sdcard_handle.Init.ClockEdge      = SDIO_CLOCK_EDGE_RISING;
    g_sdcard_handle.Init.ClockBypass    = SDIO_CLOCK_BYPASS_DISABLE;
    g_sdcard_handle.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    g_sdcard_handle.Init.BusWide        = SDIO_BUS_WIDE_1B;
    g_sdcard_handle.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    g_sdcard_handle.Init.ClockDiv       = SDIO_TRANSF_CLK_DIV;

    if (HAL_SD_Init(&g_sdcard_handle) != HAL_OK)
    {
        return 1U;
    }

    HAL_SD_GetCardInfo(&g_sdcard_handle, &g_sd_card_info_handle);

    if (HAL_SD_ConfigWideBusOperation(&g_sdcard_handle, SDIO_BUS_WIDE_4B) != HAL_OK)
    {
        return 2U;
    }

    return 0U;
}

uint8_t get_sd_card_info(HAL_SD_CardInfoTypeDef *cardinfo)
{
    return HAL_SD_GetCardInfo(&g_sdcard_handle, cardinfo);
}

void sdio_get_card_info(sd_card_info_t *info)
{
    if (info == 0)
    {
        return;
    }

    info->card_type    = g_sd_card_info_handle.CardType;
    info->block_count  = g_sd_card_info_handle.LogBlockNbr;
    info->block_size   = g_sd_card_info_handle.LogBlockSize;
    info->total_size_mb = (uint32_t)(((uint64_t)g_sd_card_info_handle.LogBlockNbr *
                                      (uint64_t)g_sd_card_info_handle.LogBlockSize) >> 20);
}

uint32_t sd_total_size_mb(void)
{
    return (uint32_t)(((uint64_t)g_sd_card_info_handle.LogBlockNbr *
                       (uint64_t)g_sd_card_info_handle.LogBlockSize) >> 20);
}

uint8_t get_sd_card_state(void)
{
    return (HAL_SD_GetCardState(&g_sdcard_handle) == HAL_SD_CARD_TRANSFER) ?
           SD_TRANSFER_OK : SD_TRANSFER_BUSY;
}

uint8_t sd_read_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    uint8_t  sta = (uint8_t)HAL_OK;
    uint32_t timeout = SD_TIMEOUT_COUNT;
    long long lsector = (long long)saddr;

    sta = (uint8_t)HAL_SD_ReadBlocks(&g_sdcard_handle, pbuf, lsector, cnt, SD_TIMEOUT_COUNT);

    while (get_sd_card_state() != SD_TRANSFER_OK)
    {
        if (timeout-- == 0U)
        {
            sta = SD_TRANSFER_BUSY;
            break;
        }
    }

    return sta;
}

uint8_t sd_write_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    uint8_t  sta = (uint8_t)HAL_OK;
    uint32_t timeout = SD_TIMEOUT_COUNT;
    long long lsector = (long long)saddr;

    sta = (uint8_t)HAL_SD_WriteBlocks(&g_sdcard_handle, pbuf, lsector, cnt, SD_TIMEOUT_COUNT);

    while (get_sd_card_state() != SD_TRANSFER_OK)
    {
        if (timeout-- == 0U)
        {
            sta = SD_TRANSFER_BUSY;
            break;
        }
    }

    return sta;
}
