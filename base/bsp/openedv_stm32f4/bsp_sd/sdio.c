/**
 * @file    sdio.c
 * @brief   SD card driver over SDIO, ported from the vendor SDIO example.
 *          MSP content (clock + GPIO) is inlined into sdio_init().
 */

#include <stdbool.h>
#include "string.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "sdio.h"

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

SD_HandleTypeDef       g_sdcard_handle;
HAL_SD_CardInfoTypeDef g_sd_card_info_handle;

static void (*s_io_hook)(void);

/* DMA is selectable at runtime through sdio_set_dma_mode(). Blocking mode is
 * the safe default; DMA is never enabled until its self-test succeeds. */

static DMA_HandleTypeDef g_sd_rx_dma;
static DMA_HandleTypeDef g_sd_tx_dma;
static SemaphoreHandle_t g_sd_dma_sem;
static volatile uint8_t  g_sd_dma_status;
static bool g_sd_use_dma;
static bool g_sd_dma_ready;
volatile uint32_t g_sd_dma_selftest_result;
volatile uint32_t g_sd_stat_rx_cb;
volatile uint32_t g_sd_stat_tx_cb;
volatile uint32_t g_sd_stat_err_cb;
volatile uint32_t g_sd_stat_sdio_irq;
volatile uint32_t g_sd_stat_dma_irq;
volatile uint32_t g_sd_stat_start_status;
volatile uint32_t g_sd_stat_error;
volatile uint32_t g_sd_stat_state;

void sdio_set_io_hook(void (*hook)(void))
{
    s_io_hook = hook;
}

static void sdio_heartbeat(void)
{
    if (s_io_hook != 0)
    {
        s_io_hook();
    }
}

void HAL_SD_RxCpltCallback(SD_HandleTypeDef *hsd)
{
    if (hsd == &g_sdcard_handle)
    {
        BaseType_t woken = pdFALSE;
        g_sd_stat_rx_cb++;
        g_sd_dma_status = 0U;
        (void)xSemaphoreGiveFromISR(g_sd_dma_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void HAL_SD_TxCpltCallback(SD_HandleTypeDef *hsd)
{
    if (hsd == &g_sdcard_handle)
    {
        BaseType_t woken = pdFALSE;
        g_sd_stat_tx_cb++;
        g_sd_dma_status = 0U;
        (void)xSemaphoreGiveFromISR(g_sd_dma_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void HAL_SD_ErrorCallback(SD_HandleTypeDef *hsd)
{
    if (hsd == &g_sdcard_handle)
    {
        BaseType_t woken = pdFALSE;
        g_sd_stat_err_cb++;
        g_sd_dma_status = 1U;
        (void)xSemaphoreGiveFromISR(g_sd_dma_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void HAL_SD_AbortCallback(SD_HandleTypeDef *hsd)
{
    if (hsd == &g_sdcard_handle)
    {
        BaseType_t woken = pdFALSE;
        g_sd_dma_status = 1U;
        (void)xSemaphoreGiveFromISR(g_sd_dma_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void SDIO_IRQHandler(void)
{
    g_sd_stat_sdio_irq++;
    HAL_SD_IRQHandler(&g_sdcard_handle);
}

void DMA2_Stream3_IRQHandler(void)
{
    g_sd_stat_dma_irq++;
    HAL_DMA_IRQHandler(&g_sd_rx_dma);
}

void DMA2_Stream6_IRQHandler(void)
{
    g_sd_stat_dma_irq++;
    HAL_DMA_IRQHandler(&g_sd_tx_dma);
}

static uint8_t sdio_dma_selftest(void);

static uint8_t sdio_dma_init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();

    g_sd_rx_dma.Instance                 = DMA2_Stream3;
    g_sd_rx_dma.Init.Channel             = DMA_CHANNEL_4;
    g_sd_rx_dma.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    g_sd_rx_dma.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_sd_rx_dma.Init.MemInc              = DMA_MINC_ENABLE;
    g_sd_rx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    g_sd_rx_dma.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    g_sd_rx_dma.Init.Mode                = DMA_NORMAL;
    g_sd_rx_dma.Init.Priority            = DMA_PRIORITY_HIGH;
    g_sd_rx_dma.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    g_sd_rx_dma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    g_sd_rx_dma.Init.MemBurst            = DMA_MBURST_SINGLE;
    g_sd_rx_dma.Init.PeriphBurst         = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&g_sd_rx_dma) != HAL_OK)
    {
        return 1U;
    }
    __HAL_LINKDMA(&g_sdcard_handle, hdmarx, g_sd_rx_dma);

    g_sd_tx_dma.Instance                 = DMA2_Stream6;
    g_sd_tx_dma.Init.Channel             = DMA_CHANNEL_4;
    g_sd_tx_dma.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    g_sd_tx_dma.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_sd_tx_dma.Init.MemInc              = DMA_MINC_ENABLE;
    g_sd_tx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    g_sd_tx_dma.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    g_sd_tx_dma.Init.Mode                = DMA_NORMAL;
    g_sd_tx_dma.Init.Priority            = DMA_PRIORITY_HIGH;
    g_sd_tx_dma.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    g_sd_tx_dma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    g_sd_tx_dma.Init.MemBurst            = DMA_MBURST_SINGLE;
    g_sd_tx_dma.Init.PeriphBurst         = DMA_PBURST_SINGLE;
    if (HAL_DMA_Init(&g_sd_tx_dma) != HAL_OK)
    {
        return 2U;
    }
    __HAL_LINKDMA(&g_sdcard_handle, hdmatx, g_sd_tx_dma);

    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
    HAL_NVIC_SetPriority(SDIO_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);
    return 0U;
}

static uint8_t sdio_dma_selftest(void)
{
    static uint8_t buf[512];
    uint8_t sta = (uint8_t)HAL_SD_ReadBlocks_DMA(&g_sdcard_handle, buf, 0, 1);

    if (sta != (uint8_t)HAL_OK)
    {
        g_sd_stat_start_status = sta;
        return 1U;
    }
    if (xSemaphoreTake(g_sd_dma_sem, pdMS_TO_TICKS(1000U)) != pdTRUE)
    {
        (void)HAL_SD_Abort(&g_sdcard_handle);
        return 2U;
    }
    return (g_sd_dma_status == 0U) ? 0U : 3U;
}

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

    if (g_sd_dma_sem == 0)
    {
        g_sd_dma_sem = xSemaphoreCreateBinary();
        if (g_sd_dma_sem == 0)
        {
            return 3U;
        }
    }

    g_sd_dma_ready = false;
    g_sd_use_dma = false;

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

static void sdio_recover(void)
{
    /* Do not re-run HAL_SD_Init() from a failed data transfer: re-entering
     * SD_FindSCR while the card/USB path is active has caused HardFaults.
     * Abort the current data path and let the caller retry in blocking mode. */
    (void)HAL_SD_Abort(&g_sdcard_handle);
    HAL_Delay(20U);
}
static uint8_t sd_read_disk_blocking(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    uint8_t sta = (uint8_t)HAL_ERROR;

    for (uint8_t attempt = 0U; attempt < 3U; attempt++)
    {
        sdio_heartbeat();
        sta = (uint8_t)HAL_SD_ReadBlocks(&g_sdcard_handle, pbuf, (long long)saddr,
                                          cnt, SD_TIMEOUT_COUNT);
        sdio_heartbeat();
        if (sta == (uint8_t)HAL_OK)
        {
            sta = (get_sd_card_state() == SD_TRANSFER_OK) ? 0U : SD_TRANSFER_BUSY;
        }
        if (sta == 0U)
        {
            return 0U;
        }
        g_sd_stat_error = g_sdcard_handle.ErrorCode;
        g_sd_stat_state = g_sdcard_handle.State;
        if (attempt == 0U)
        {
            sdio_recover();
        }
        HAL_Delay(50U);
    }
    return sta;
}

static uint8_t sd_write_disk_blocking(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    uint8_t sta = (uint8_t)HAL_ERROR;

    for (uint8_t attempt = 0U; attempt < 3U; attempt++)
    {
        sdio_heartbeat();
        sta = (uint8_t)HAL_SD_WriteBlocks(&g_sdcard_handle, pbuf, (long long)saddr,
                                           cnt, SD_TIMEOUT_COUNT);
        sdio_heartbeat();
        if (sta == (uint8_t)HAL_OK)
        {
            sta = (get_sd_card_state() == SD_TRANSFER_OK) ? 0U : SD_TRANSFER_BUSY;
        }
        if (sta == 0U)
        {
            return 0U;
        }
        g_sd_stat_error = g_sdcard_handle.ErrorCode;
        g_sd_stat_state = g_sdcard_handle.State;
        if (attempt == 0U)
        {
            sdio_recover();
        }
        HAL_Delay(50U);
    }
    return sta;
}

static uint8_t sdio_wait_dma(void)
{
    TickType_t ticks = pdMS_TO_TICKS(SD_TIMEOUT_COUNT);

    sdio_heartbeat();
    if (xSemaphoreTake(g_sd_dma_sem, ticks) != pdTRUE)
    {
        sdio_heartbeat();
        return SD_TRANSFER_BUSY;
    }
    sdio_heartbeat();
    return (g_sd_dma_status == 0U) ? 0U : (uint8_t)HAL_ERROR;
}

int sdio_set_dma_mode(bool enable)
{
    if (!enable)
    {
        g_sd_use_dma = false;
        return 0;
    }

    if (g_sd_use_dma)
    {
        return 0;
    }

    if (!g_sd_dma_ready)
    {
        if (sdio_dma_init() != 0U)
        {
            return -1;
        }
        g_sd_dma_ready = true;
    }

    g_sd_dma_selftest_result = sdio_dma_selftest();
    if (g_sd_dma_selftest_result != 0U)
    {
        g_sd_use_dma = false;
        return -1;
    }

    g_sd_use_dma = true;
    return 0;
}

bool sdio_get_dma_mode(void)
{
    return g_sd_use_dma;
}

uint8_t sd_read_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    uint8_t sta = (uint8_t)HAL_OK;

    if (!g_sd_use_dma || (((uintptr_t)pbuf & 0x3U) != 0U))
    {
        return sd_read_disk_blocking(pbuf, saddr, cnt);
    }

    for (uint8_t attempt = 0U; attempt < 3U; attempt++)
    {
        (void)xSemaphoreTake(g_sd_dma_sem, 0U);
        g_sd_dma_status = 1U;
        sdio_heartbeat();
        sta = (uint8_t)HAL_SD_ReadBlocks_DMA(&g_sdcard_handle, pbuf,
                                             (long long)saddr, cnt);
        g_sd_stat_start_status = sta;
        if (sta == (uint8_t)HAL_OK)
        {
            sta = sdio_wait_dma();
        }
        else
        {
            sdio_heartbeat();
        }
        if (sta == 0U)
        {
            return 0U;
        }
        g_sd_stat_error = g_sdcard_handle.ErrorCode;
        g_sd_stat_state = g_sdcard_handle.State;
        (void)HAL_SD_Abort(&g_sdcard_handle);
        g_sd_use_dma = false;
        return sd_read_disk_blocking(pbuf, saddr, cnt);
    }
    return sta;
}

uint8_t sd_write_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    uint8_t sta = (uint8_t)HAL_OK;

    if (!g_sd_use_dma || (((uintptr_t)pbuf & 0x3U) != 0U))
    {
        return sd_write_disk_blocking(pbuf, saddr, cnt);
    }

    for (uint8_t attempt = 0U; attempt < 3U; attempt++)
    {
        (void)xSemaphoreTake(g_sd_dma_sem, 0U);
        g_sd_dma_status = 1U;
        sdio_heartbeat();
        sta = (uint8_t)HAL_SD_WriteBlocks_DMA(&g_sdcard_handle, pbuf,
                                              (long long)saddr, cnt);
        g_sd_stat_start_status = sta;
        if (sta == (uint8_t)HAL_OK)
        {
            sta = sdio_wait_dma();
        }
        else
        {
            sdio_heartbeat();
        }
        if (sta == 0U)
        {
            return 0U;
        }
        g_sd_stat_error = g_sdcard_handle.ErrorCode;
        g_sd_stat_state = g_sdcard_handle.State;
        (void)HAL_SD_Abort(&g_sdcard_handle);
        g_sd_use_dma = false;
        return sd_write_disk_blocking(pbuf, saddr, cnt);
    }
    return sta;
}
