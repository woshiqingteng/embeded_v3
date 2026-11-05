#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "bsp_sd.h"
#include "bsp_log.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_sdmmc.h"

#define SDIO_TRANSF_CLK_DIV   22
#define SD_TIMEOUT_COUNT      ((uint32_t)3000)
#define SD_TRANSFER_OK        ((uint8_t)0x00)
#define SD_TRANSFER_BUSY      ((uint8_t)0x01)

uint8_t sdio_init(void);
void sdio_set_io_hook(void (*hook)(void));
int  sdio_set_dma_mode(bool enable);
bool sdio_get_dma_mode(void);
void sdio_get_card_info(sd_card_info_t *info);
uint8_t sd_read_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt);
uint8_t sd_write_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt);

static SemaphoreHandle_t g_sd_mutex;
static bsp_sd_owner_t    g_owner = BSP_SD_OWNER_NONE;
static bsp_sd_io_hook_t  g_io_hook;
static uint8_t           g_ready;

extern HAL_SD_CardInfoTypeDef g_sd_card_info_handle;

static bool bsp_sd_range_valid(uint32_t lba, uint32_t count)
{
    uint32_t blocks = g_sd_card_info_handle.LogBlockNbr;

    if ((blocks == 0U) || (count == 0U) || (lba >= blocks))
    {
        return false;
    }
    return (count <= (blocks - lba));
}

int bsp_sd_init(void)
{
    if (g_ready != 0U)
    {
        return 0;
    }

    if (g_sd_mutex == 0)
    {
        g_sd_mutex = xSemaphoreCreateMutex();
    }
    if (g_sd_mutex == 0)
    {
        return -1;
    }

    g_ready = (sdio_init() == 0U) ? 1U : 0U;
    if (g_ready != 0U)
    {
        sd_card_info_t info;

        if (bsp_sd_get_info(&info) == 0)
        {
            BSP_LOGI("sd", "size=%lu MiB blocks=%lu block_size=%lu B readonly=%u",
                     (unsigned long)info.total_size_mb,
                     (unsigned long)info.block_count,
                     (unsigned long)info.block_size,
                     (unsigned)(bsp_sd_is_writable() ? 0U : 1U));
        }
    }
    else
    {
        BSP_LOGE("sd", "init failed; storage read-only");
    }
    return (g_ready != 0U) ? 0 : -1;
}

int bsp_sd_get_info(sd_card_info_t *info)
{
    if ((info == 0) || (g_ready == 0U))
    {
        return -1;
    }
    sdio_get_card_info(info);
    return 0;
}

int bsp_sd_acquire(bsp_sd_owner_t owner, uint32_t timeout_ms)
{
    TickType_t ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

    if ((g_sd_mutex == 0) || (g_ready == 0U))
    {
        return -1;
    }
    if (xSemaphoreTake(g_sd_mutex, ticks) != pdTRUE)
    {
        return -1;
    }

    if ((g_owner != BSP_SD_OWNER_NONE) && (g_owner != owner))
    {
        (void)xSemaphoreGive(g_sd_mutex);
        return -1;
    }

    g_owner = owner;
    (void)xSemaphoreGive(g_sd_mutex);
    return 0;
}

void bsp_sd_release(bsp_sd_owner_t owner)
{
    if ((g_sd_mutex == 0) || (owner == BSP_SD_OWNER_NONE))
    {
        return;
    }
    if (xSemaphoreTake(g_sd_mutex, portMAX_DELAY) == pdTRUE)
    {
        if (g_owner == owner)
        {
            g_owner = BSP_SD_OWNER_NONE;
        }
        (void)xSemaphoreGive(g_sd_mutex);
    }
}

bsp_sd_owner_t bsp_sd_owner(void)
{
    bsp_sd_owner_t owner = BSP_SD_OWNER_NONE;

    if (g_sd_mutex != 0 && xSemaphoreTake(g_sd_mutex, portMAX_DELAY) == pdTRUE)
    {
        owner = g_owner;
        (void)xSemaphoreGive(g_sd_mutex);
    }
    return owner;
}

bool bsp_sd_is_writable(void)
{
    /* Never probe by writing: the last sector may contain filesystem data. */
    return (BSP_SD_WRITABLE != 0);
}

void bsp_sd_set_io_hook(bsp_sd_io_hook_t hook)
{
    g_io_hook = hook;
    sdio_set_io_hook(hook);
}

int bsp_sd_set_dma_enabled(bool enable)
{
    int ret;

    if ((g_sd_mutex == 0) || (g_ready == 0U) ||
        (xSemaphoreTake(g_sd_mutex, pdMS_TO_TICKS(1000U)) != pdTRUE))
    {
        return -1;
    }
    ret = sdio_set_dma_mode(enable);
    (void)xSemaphoreGive(g_sd_mutex);
    return ret;
}

bool bsp_sd_dma_enabled(void)
{
    return sdio_get_dma_mode();
}

extern volatile uint32_t g_sd_dma_selftest_result;
extern volatile uint32_t g_sd_stat_sdio_irq;
extern volatile uint32_t g_sd_stat_dma_irq;
extern volatile uint32_t g_sd_stat_error;
extern volatile uint32_t g_sd_stat_start_status;

void bsp_sd_get_diag(bsp_sd_diag_t *diag)
{
    if (diag == 0)
    {
        return;
    }
    diag->selftest = g_sd_dma_selftest_result;
    diag->sdio_irq = g_sd_stat_sdio_irq;
    diag->dma_irq = g_sd_stat_dma_irq;
    diag->error = g_sd_stat_error;
    diag->start_status = g_sd_stat_start_status;
}

int bsp_sd_read(uint8_t *buf, uint32_t lba, uint32_t count)
{
    int ret = -1;

    if ((buf == 0) || (g_sd_mutex == 0) || (g_ready == 0U) ||
        !bsp_sd_range_valid(lba, count))
    {
        return -1;
    }
    if (xSemaphoreTake(g_sd_mutex, portMAX_DELAY) != pdTRUE)
    {
        return -1;
    }
    if (g_owner == BSP_SD_OWNER_NONE)
    {
        (void)xSemaphoreGive(g_sd_mutex);
        return -1;
    }

    if (g_io_hook != 0)
    {
        g_io_hook();
    }
    ret = (sd_read_disk(buf, lba, count) == 0U) ? 0 : -1;
    if (g_io_hook != 0)
    {
        g_io_hook();
    }
    (void)xSemaphoreGive(g_sd_mutex);
    return ret;
}

int bsp_sd_write(const uint8_t *buf, uint32_t lba, uint32_t count)
{
    if ((buf == 0) || (g_sd_mutex == 0) || (g_ready == 0U) ||
        !bsp_sd_range_valid(lba, count))
    {
        return -1;
    }
#if (BSP_SD_WRITABLE == 0)
    (void)lba;
    return -1;
#else
    int ret = -1;

    if (xSemaphoreTake(g_sd_mutex, portMAX_DELAY) != pdTRUE)
    {
        return -1;
    }
    if (g_owner == BSP_SD_OWNER_NONE)
    {
        (void)xSemaphoreGive(g_sd_mutex);
        return -1;
    }

    if (g_io_hook != 0)
    {
        g_io_hook();
    }
    ret = (sd_write_disk((uint8_t *)buf, lba, count) == 0U) ? 0 : -1;
    if (g_io_hook != 0)
    {
        g_io_hook();
    }
    (void)xSemaphoreGive(g_sd_mutex);
    return ret;
#endif
}


/* ---- SDIO low-level driver ---- */
/**
 * @file    sdio.c
 * @brief   SD card driver over SDIO.  HAL is used only for card/controller
 *          initialisation; all data transfers and interrupt completion use
 *          explicit HAL flag get/set macros.
 */




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

#define SD_BLOCK_BYTES     512U
#define SDIO_DMA_USE_HAL  1
#define SD_BLOCK_WORDS     (SD_BLOCK_BYTES / 4U)

SD_HandleTypeDef       g_sdcard_handle;
HAL_SD_CardInfoTypeDef g_sd_card_info_handle;

static void (*s_io_hook)(void);

static DMA_HandleTypeDef g_sd_rx_dma;
static DMA_HandleTypeDef g_sd_tx_dma;
static SemaphoreHandle_t g_sd_dma_sem;
static volatile uint8_t  g_sd_dma_status;
static volatile bool     g_sd_data_active;
static bool              g_sd_dma_read;
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

static uint32_t sdio_card_address(uint32_t saddr)
{
    return (g_sd_card_info_handle.CardType == CARD_SDHC_SDXC)
               ? saddr : (saddr * SD_BLOCK_BYTES);
}

static bool sdio_has_error(SD_HandleTypeDef *hsd)
{
    if (__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_DCRCFAIL) != RESET) return true;
    if (__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_DTIMEOUT) != RESET) return true;
    if (__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_RXOVERR) != RESET) return true;
    if (__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_TXUNDERR) != RESET) return true;
#ifdef SDIO_FLAG_STBITERR
    if (__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_STBITERR) != RESET) return true;
#endif
    return false;
}

static uint32_t sdio_error_value(SD_HandleTypeDef *hsd)
{
    uint32_t value = 0U;

    if (__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_DCRCFAIL) != RESET) value |= SDIO_FLAG_DCRCFAIL;
    if (__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_DTIMEOUT) != RESET) value |= SDIO_FLAG_DTIMEOUT;
    if (__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_RXOVERR) != RESET) value |= SDIO_FLAG_RXOVERR;
    if (__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_TXUNDERR) != RESET) value |= SDIO_FLAG_TXUNDERR;
#ifdef SDIO_FLAG_STBITERR
    if (__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_STBITERR) != RESET) value |= SDIO_FLAG_STBITERR;
#endif
    return value;
}

static void sdio_disarm_data_irq(void)
{
    __HAL_SD_DISABLE_IT(&g_sdcard_handle,
                        SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT |
                        SDIO_IT_RXOVERR | SDIO_IT_TXUNDERR |
                        SDIO_IT_DATAEND | SDIO_IT_STBITERR);
    HAL_NVIC_DisableIRQ(SDIO_IRQn);
}

static uint8_t sdio_wait_card_ready(uint32_t timeout_ms);

static void sdio_flush_fifo(void)
{
    uint32_t guard = 256U;

    while (((SDIO->FIFOCNT != 0U) ||
            (__HAL_SD_GET_FLAG(&g_sdcard_handle, SDIO_FLAG_RXDAVL) != RESET)) &&
           (guard-- != 0U))
    {
        (void)SDIO->FIFO;
    }
}

__attribute__((unused)) static void sdio_reset_data_path(void)
{
    __HAL_SD_DMA_DISABLE(&g_sdcard_handle);
    CLEAR_BIT(SDIO->DCTRL, SDIO_DCTRL_DTEN | SDIO_DCTRL_DMAEN);
    sdio_flush_fifo();
    __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_FLAGS);
}

__attribute__((unused)) static uint8_t sdio_send_data_cmd(uint8_t cmd_index, uint32_t argument)
{
    SDIO_CmdInitTypeDef cmd;

    cmd.Argument         = argument;
    cmd.CmdIndex         = cmd_index;
    cmd.Response         = SDIO_RESPONSE_SHORT;
    cmd.WaitForInterrupt = SDIO_WAIT_NO;
    cmd.CPSM             = SDIO_CPSM_ENABLE;
    return (SDIO_SendCommand(SDIO, &cmd) == HAL_OK) ? 0U : 1U;
}

__attribute__((unused)) static uint8_t sdio_wait_cmd_response(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();

    while ((__HAL_SD_GET_FLAG(&g_sdcard_handle, SDIO_FLAG_CMDREND) == RESET) &&
           (__HAL_SD_GET_FLAG(&g_sdcard_handle, SDIO_FLAG_CCRCFAIL) == RESET) &&
           (__HAL_SD_GET_FLAG(&g_sdcard_handle, SDIO_FLAG_CTIMEOUT) == RESET))
    {
        if ((HAL_GetTick() - start) > timeout_ms)
        {
            g_sd_stat_error = SDIO_FLAG_CTIMEOUT;
            __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_CMD_FLAGS);
            return 2U;
        }
    }

    if ((__HAL_SD_GET_FLAG(&g_sdcard_handle, SDIO_FLAG_CCRCFAIL) != RESET) ||
        (__HAL_SD_GET_FLAG(&g_sdcard_handle, SDIO_FLAG_CTIMEOUT) != RESET))
    {
        g_sd_stat_error = sdio_error_value(&g_sdcard_handle) |
                          (__HAL_SD_GET_FLAG(&g_sdcard_handle, SDIO_FLAG_CCRCFAIL) != RESET
                               ? SDIO_FLAG_CCRCFAIL : SDIO_FLAG_CTIMEOUT);
        __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_CMD_FLAGS);
        return 1U;
    }

    __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_CMD_FLAGS);
    return 0U;
}

__attribute__((unused)) static void sdio_dma_abort(void)
{
    __HAL_DMA_DISABLE_IT(&g_sd_rx_dma, DMA_IT_TC | DMA_IT_TE | DMA_IT_DME);
    __HAL_DMA_DISABLE_IT(&g_sd_tx_dma, DMA_IT_TC | DMA_IT_TE | DMA_IT_DME);
    __HAL_DMA_DISABLE(&g_sd_rx_dma);
    __HAL_DMA_DISABLE(&g_sd_tx_dma);
    g_sd_rx_dma.Instance->CR &= ~DMA_SxCR_EN;
    g_sd_tx_dma.Instance->CR &= ~DMA_SxCR_EN;

    sdio_disarm_data_irq();
    __HAL_SD_DMA_DISABLE(&g_sdcard_handle);
    CLEAR_BIT(SDIO->DCTRL, SDIO_DCTRL_DTEN | SDIO_DCTRL_DMAEN);
    sdio_flush_fifo();
    __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_FLAGS);

    (void)SDMMC_CmdStopTransfer(SDIO);
    (void)sdio_wait_card_ready(100U);

    g_sd_rx_dma.State = HAL_DMA_STATE_READY;
    g_sd_tx_dma.State = HAL_DMA_STATE_READY;
    g_sd_data_active = false;
}

void SDIO_IRQHandler(void)
{
    SD_HandleTypeDef *hsd = &g_sdcard_handle;

    g_sd_stat_sdio_irq++;
#if SDIO_DMA_USE_HAL
    HAL_SD_IRQHandler(hsd);
    return;
#endif

    if (!g_sd_data_active)
    {
        __HAL_SD_CLEAR_FLAG(hsd, SDIO_STATIC_FLAGS);
        return;
    }

    if (sdio_has_error(hsd) ||
        (__HAL_SD_GET_FLAG(hsd, SDIO_FLAG_DATAEND) != RESET))
    {
        BaseType_t woken = pdFALSE;

        if (sdio_has_error(hsd))
        {
            g_sd_stat_error = sdio_error_value(hsd);
            g_sd_dma_status = 1U;
        }
        else
        {
            g_sd_dma_status = 0U;
        }

        if (g_sd_dma_status != 0U)
        {
            __HAL_SD_DMA_DISABLE(hsd);
        }
        sdio_disarm_data_irq();
        __HAL_SD_CLEAR_FLAG(hsd, SDIO_STATIC_DATA_FLAGS);
        g_sd_data_active = false;
        (void)xSemaphoreGiveFromISR(g_sd_dma_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void DMA2_Stream3_IRQHandler(void)
{
    g_sd_stat_dma_irq++;
#if SDIO_DMA_USE_HAL
    HAL_DMA_IRQHandler(&g_sd_rx_dma);
    return;
#endif
    if (__HAL_DMA_GET_FLAG(&g_sd_rx_dma, DMA_FLAG_TCIF3_7) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(&g_sd_rx_dma, DMA_FLAG_TCIF3_7);
    }
    if ((__HAL_DMA_GET_FLAG(&g_sd_rx_dma, DMA_FLAG_TEIF3_7) != RESET) ||
        (__HAL_DMA_GET_FLAG(&g_sd_rx_dma, DMA_FLAG_DMEIF3_7) != RESET) ||
        (__HAL_DMA_GET_FLAG(&g_sd_rx_dma, DMA_FLAG_FEIF3_7) != RESET))
    {
        BaseType_t woken = pdFALSE;

        g_sd_stat_error = DMA2->LISR;
        __HAL_DMA_CLEAR_FLAG(&g_sd_rx_dma,
                             DMA_FLAG_TEIF3_7 | DMA_FLAG_DMEIF3_7 | DMA_FLAG_FEIF3_7);
        g_sd_dma_status = 1U;
        __HAL_DMA_DISABLE(&g_sd_rx_dma);
        __HAL_SD_DMA_DISABLE(&g_sdcard_handle);
        sdio_disarm_data_irq();
        __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_DATA_FLAGS);
        g_sd_data_active = false;
        (void)xSemaphoreGiveFromISR(g_sd_dma_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

void DMA2_Stream6_IRQHandler(void)
{
    g_sd_stat_dma_irq++;
#if SDIO_DMA_USE_HAL
    HAL_DMA_IRQHandler(&g_sd_tx_dma);
    return;
#endif
    if (__HAL_DMA_GET_FLAG(&g_sd_tx_dma, DMA_FLAG_TCIF2_6) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(&g_sd_tx_dma, DMA_FLAG_TCIF2_6);
    }
    if ((__HAL_DMA_GET_FLAG(&g_sd_tx_dma, DMA_FLAG_TEIF2_6) != RESET) ||
        (__HAL_DMA_GET_FLAG(&g_sd_tx_dma, DMA_FLAG_DMEIF2_6) != RESET) ||
        (__HAL_DMA_GET_FLAG(&g_sd_tx_dma, DMA_FLAG_FEIF2_6) != RESET))
    {
        BaseType_t woken = pdFALSE;

        __HAL_DMA_CLEAR_FLAG(&g_sd_tx_dma,
                             DMA_FLAG_TEIF2_6 | DMA_FLAG_DMEIF2_6 | DMA_FLAG_FEIF2_6);
        g_sd_dma_status = 1U;
        __HAL_DMA_DISABLE(&g_sd_tx_dma);
        __HAL_SD_DMA_DISABLE(&g_sdcard_handle);
        sdio_disarm_data_irq();
        __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_DATA_FLAGS);
        g_sd_data_active = false;
        (void)xSemaphoreGiveFromISR(g_sd_dma_sem, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

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
    g_sd_rx_dma.Init.Mode                = DMA_PFCTRL;
    g_sd_rx_dma.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
    g_sd_rx_dma.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    g_sd_rx_dma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    g_sd_rx_dma.Init.MemBurst            = DMA_MBURST_INC4;
    g_sd_rx_dma.Init.PeriphBurst         = DMA_PBURST_INC4;
    if (HAL_DMA_Init(&g_sd_rx_dma) != HAL_OK)
    {
        return 1U;
    }

    g_sd_tx_dma.Instance                 = DMA2_Stream6;
    g_sd_tx_dma.Init.Channel             = DMA_CHANNEL_4;
    g_sd_tx_dma.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    g_sd_tx_dma.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_sd_tx_dma.Init.MemInc              = DMA_MINC_ENABLE;
    g_sd_tx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    g_sd_tx_dma.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    g_sd_tx_dma.Init.Mode                = DMA_PFCTRL;
    g_sd_tx_dma.Init.Priority            = DMA_PRIORITY_VERY_HIGH;
    g_sd_tx_dma.Init.FIFOMode            = DMA_FIFOMODE_ENABLE;
    g_sd_tx_dma.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    g_sd_tx_dma.Init.MemBurst            = DMA_MBURST_INC4;
    g_sd_tx_dma.Init.PeriphBurst         = DMA_PBURST_INC4;
    if (HAL_DMA_Init(&g_sd_tx_dma) != HAL_OK)
    {
        return 2U;
    }

    __HAL_LINKDMA(&g_sdcard_handle, hdmarx, g_sd_rx_dma);
    __HAL_LINKDMA(&g_sdcard_handle, hdmatx, g_sd_tx_dma);

    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 5U, 0U);
    HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 5U, 0U);
    HAL_NVIC_SetPriority(SDIO_IRQn, 6U, 0U);
    return 0U;
}

static uint8_t sdio_wait_card_ready(uint32_t timeout_ms)
{
    uint32_t start = HAL_GetTick();
    uint32_t argument = g_sd_card_info_handle.RelCardAdd << 16;

    while ((HAL_GetTick() - start) < timeout_ms)
    {
        uint32_t status;

        if (SDMMC_CmdSendStatus(SDIO, argument) != SDMMC_ERROR_NONE)
        {
            return 1U;
        }
        status = SDIO_GetResponse(SDIO, SDIO_RESP1);
        if (((status >> 9U) & 0x0FU) == 4U)
        {
            return 0U;
        }
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
    return 1U;
}

static uint8_t sdio_read_blocking(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    SDIO_DataInitTypeDef cfg;
    uint32_t addr = sdio_card_address(saddr);
    uint32_t remaining = cnt * SD_BLOCK_BYTES;
    uint8_t *p = pbuf;
    uint32_t start = HAL_GetTick();

    __HAL_SD_DMA_DISABLE(&g_sdcard_handle);
    __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_FLAGS);

    cfg.DataTimeOut = SDMMC_DATATIMEOUT;
    cfg.DataLength = cnt * SD_BLOCK_BYTES;
    cfg.DataBlockSize = SDIO_DATABLOCK_SIZE_512B;
    cfg.TransferDir = SDIO_TRANSFER_DIR_TO_SDIO;
    cfg.TransferMode = SDIO_TRANSFER_MODE_BLOCK;
    cfg.DPSM = SDIO_DPSM_ENABLE;
    (void)SDIO_ConfigData(SDIO, &cfg);

    if ((cnt > 1U) ? (SDMMC_CmdReadMultiBlock(SDIO, addr) != SDMMC_ERROR_NONE)
                   : (SDMMC_CmdReadSingleBlock(SDIO, addr) != SDMMC_ERROR_NONE))
    {
        __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_FLAGS);
        return 1U;
    }

    while (remaining > 0U)
    {
        if (sdio_has_error(&g_sdcard_handle))
        {
            g_sd_stat_error = sdio_error_value(&g_sdcard_handle);
            __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_DATA_FLAGS);
            return 1U;
        }
        if (__HAL_SD_GET_FLAG(&g_sdcard_handle, SDIO_FLAG_RXDAVL) != RESET)
        {
            uint32_t word = SDIO_ReadFIFO(SDIO);
            memcpy(p, &word, 4U);
            p += 4U;
            remaining -= 4U;
        }
        if ((HAL_GetTick() - start) > SD_TIMEOUT_COUNT)
        {
            __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_DATA_FLAGS);
            return 2U;
        }
    }

    while (__HAL_SD_GET_FLAG(&g_sdcard_handle, SDIO_FLAG_DATAEND) == RESET)
    {
        if (sdio_has_error(&g_sdcard_handle) ||
            ((HAL_GetTick() - start) > SD_TIMEOUT_COUNT))
        {
            g_sd_stat_error = sdio_error_value(&g_sdcard_handle);
            __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_DATA_FLAGS);
            return 1U;
        }
    }

    if (cnt > 1U)
    {
        (void)SDMMC_CmdStopTransfer(SDIO);
    }
    __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_DATA_FLAGS);
    return 0U;
}

static uint8_t sdio_write_blocking(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    SDIO_DataInitTypeDef cfg;
    uint32_t addr = sdio_card_address(saddr);
    uint32_t remaining = cnt * SD_BLOCK_BYTES;
    uint8_t *p = pbuf;
    uint32_t start = HAL_GetTick();

    __HAL_SD_DMA_DISABLE(&g_sdcard_handle);
    __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_FLAGS);

    cfg.DataTimeOut = SDMMC_DATATIMEOUT;
    cfg.DataLength = cnt * SD_BLOCK_BYTES;
    cfg.DataBlockSize = SDIO_DATABLOCK_SIZE_512B;
    cfg.TransferDir = SDIO_TRANSFER_DIR_TO_CARD;
    cfg.TransferMode = SDIO_TRANSFER_MODE_BLOCK;
    cfg.DPSM = SDIO_DPSM_ENABLE;
    (void)SDIO_ConfigData(SDIO, &cfg);

    if ((cnt > 1U) ? (SDMMC_CmdWriteMultiBlock(SDIO, addr) != SDMMC_ERROR_NONE)
                   : (SDMMC_CmdWriteSingleBlock(SDIO, addr) != SDMMC_ERROR_NONE))
    {
        __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_FLAGS);
        return 1U;
    }

    while (remaining > 0U)
    {
        uint32_t word;

        if (sdio_has_error(&g_sdcard_handle))
        {
            g_sd_stat_error = sdio_error_value(&g_sdcard_handle);
            __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_DATA_FLAGS);
            return 1U;
        }
        if (__HAL_SD_GET_FLAG(&g_sdcard_handle, SDIO_FLAG_TXDAVL) != RESET)
        {
            memcpy(&word, p, 4U);
            (void)SDIO_WriteFIFO(SDIO, &word);
            p += 4U;
            remaining -= 4U;
        }
        if ((HAL_GetTick() - start) > SD_TIMEOUT_COUNT)
        {
            __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_DATA_FLAGS);
            return 2U;
        }
    }

    while (__HAL_SD_GET_FLAG(&g_sdcard_handle, SDIO_FLAG_DATAEND) == RESET)
    {
        if (sdio_has_error(&g_sdcard_handle) ||
            ((HAL_GetTick() - start) > SD_TIMEOUT_COUNT))
        {
            g_sd_stat_error = sdio_error_value(&g_sdcard_handle);
            __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_DATA_FLAGS);
            return 1U;
        }
    }

    if (cnt > 1U)
    {
        (void)SDMMC_CmdStopTransfer(SDIO);
    }
    __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_DATA_FLAGS);
    return sdio_wait_card_ready(SD_TIMEOUT_COUNT);
}

__attribute__((unused)) static uint8_t sdio_wait_dma(void)
{
    TickType_t ticks = pdMS_TO_TICKS(SD_TIMEOUT_COUNT);
    uint32_t start;
    uint32_t tc_flag = g_sd_dma_read ? DMA_FLAG_TCIF3_7 : DMA_FLAG_TCIF2_6;
    DMA_HandleTypeDef *hdma = g_sd_dma_read ? &g_sd_rx_dma : &g_sd_tx_dma;

    sdio_heartbeat();
    if (xSemaphoreTake(g_sd_dma_sem, ticks) != pdTRUE)
    {
        sdio_dma_abort();
        sdio_heartbeat();
        return SD_TRANSFER_BUSY;
    }
    sdio_heartbeat();
    if (g_sd_dma_status != 0U)
    {
        sdio_dma_abort();
        return (uint8_t)HAL_ERROR;
    }

    start = HAL_GetTick();
    while (__HAL_DMA_GET_FLAG(hdma, tc_flag) == RESET)
    {
        if ((HAL_GetTick() - start) > SD_TIMEOUT_COUNT)
        {
            sdio_dma_abort();
            return (uint8_t)HAL_ERROR;
        }
    }

    __HAL_DMA_CLEAR_FLAG(hdma, tc_flag);
    __HAL_DMA_DISABLE(hdma);
    __HAL_DMA_DISABLE_IT(hdma, DMA_IT_TC | DMA_IT_TE | DMA_IT_DME);
    hdma->State = HAL_DMA_STATE_READY;
    __HAL_SD_DMA_DISABLE(&g_sdcard_handle);
    return 0U;
}



static uint8_t sdio_transfer_dma(bool read, uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
#if SDIO_DMA_USE_HAL
    uint32_t addr = sdio_card_address(saddr);
    uint32_t start;
    HAL_StatusTypeDef st;

    (void)xSemaphoreTake(g_sd_dma_sem, 0U);
    g_sd_dma_status = 1U;
    g_sd_data_active = true;
    g_sd_dma_read = read;
    g_sd_stat_error = 0U;
    g_sd_stat_start_status = 0U;
    __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_FLAGS);
    HAL_NVIC_ClearPendingIRQ(SDIO_IRQn);
    HAL_NVIC_ClearPendingIRQ(read ? DMA2_Stream3_IRQn : DMA2_Stream6_IRQn);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);
    HAL_NVIC_EnableIRQ(read ? DMA2_Stream3_IRQn : DMA2_Stream6_IRQn);

    st = read ? HAL_SD_ReadBlocks_DMA(&g_sdcard_handle, pbuf, addr, cnt)
              : HAL_SD_WriteBlocks_DMA(&g_sdcard_handle, pbuf, addr, cnt);
    if (st != HAL_OK)
    {
        HAL_NVIC_DisableIRQ(SDIO_IRQn);
        HAL_NVIC_DisableIRQ(read ? DMA2_Stream3_IRQn : DMA2_Stream6_IRQn);
        g_sd_data_active = false;
        return 1U;
    }

    start = HAL_GetTick();
    while ((g_sdcard_handle.State != HAL_SD_STATE_READY) &&
           (g_sdcard_handle.State != HAL_SD_STATE_ERROR))
    {
        if ((HAL_GetTick() - start) > SD_TIMEOUT_COUNT)
        {
            (void)HAL_SD_Abort(&g_sdcard_handle);
            HAL_NVIC_DisableIRQ(SDIO_IRQn);
            HAL_NVIC_DisableIRQ(read ? DMA2_Stream3_IRQn : DMA2_Stream6_IRQn);
            g_sd_data_active = false;
            return 2U;
        }
        vTaskDelay(pdMS_TO_TICKS(1U));
    }

    HAL_NVIC_DisableIRQ(SDIO_IRQn);
    HAL_NVIC_DisableIRQ(read ? DMA2_Stream3_IRQn : DMA2_Stream6_IRQn);
    HAL_NVIC_ClearPendingIRQ(SDIO_IRQn);
    HAL_NVIC_ClearPendingIRQ(read ? DMA2_Stream3_IRQn : DMA2_Stream6_IRQn);
    __HAL_SD_DISABLE_IT(&g_sdcard_handle, SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT |
                        SDIO_IT_RXOVERR | SDIO_IT_TXUNDERR |
                        SDIO_IT_DATAEND | SDIO_IT_STBITERR);
    __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_FLAGS);
    g_sd_data_active = false;
    if (g_sdcard_handle.State == HAL_SD_STATE_ERROR)
    {
        g_sd_stat_error = g_sdcard_handle.ErrorCode;
        return 1U;
    }
    return 0U;
#else
    SD_HandleTypeDef *hsd = &g_sdcard_handle;
    DMA_HandleTypeDef *hdma = read ? &g_sd_rx_dma : &g_sd_tx_dma;
    SDIO_DataInitTypeDef cfg;
    uint32_t addr = sdio_card_address(saddr);
    uint32_t words = cnt * SD_BLOCK_WORDS;

    if ((words == 0U) || (words > 0xFFFFU) || (((uintptr_t)pbuf & 0x3U) != 0U))
    {
        return 3U;
    }

    (void)xSemaphoreTake(g_sd_dma_sem, 0U);
    g_sd_dma_status = 1U;
    g_sd_data_active = true;
    g_sd_dma_read = read;

    sdio_reset_data_path();
    __HAL_DMA_DISABLE_IT(hdma, DMA_IT_TC | DMA_IT_TE | DMA_IT_DME);
    __HAL_DMA_DISABLE(hdma);
    hdma->State = HAL_DMA_STATE_READY;
    if (read)
    {
        __HAL_DMA_CLEAR_FLAG(hdma, DMA_FLAG_TCIF3_7 | DMA_FLAG_HTIF3_7 |
                                    DMA_FLAG_TEIF3_7 | DMA_FLAG_DMEIF3_7 | DMA_FLAG_FEIF3_7);
    }
    else
    {
        __HAL_DMA_CLEAR_FLAG(hdma, DMA_FLAG_TCIF2_6 | DMA_FLAG_HTIF2_6 |
                                    DMA_FLAG_TEIF2_6 | DMA_FLAG_DMEIF2_6 | DMA_FLAG_FEIF2_6);
    }
    if (HAL_DMA_Start_IT(hdma, (uint32_t)&SDIO->FIFO, (uint32_t)pbuf, words) != HAL_OK)
    {
        g_sd_data_active = false;
        return 1U;
    }

    __HAL_SD_DMA_ENABLE(hsd);

    cfg.DataTimeOut = SDMMC_DATATIMEOUT;
    cfg.DataLength = cnt * SD_BLOCK_BYTES;
    cfg.DataBlockSize = SDIO_DATABLOCK_SIZE_512B;
    cfg.TransferDir = read ? SDIO_TRANSFER_DIR_TO_SDIO : SDIO_TRANSFER_DIR_TO_CARD;
    cfg.TransferMode = SDIO_TRANSFER_MODE_BLOCK;
    cfg.DPSM = SDIO_DPSM_ENABLE;
    (void)SDIO_ConfigData(SDIO, &cfg);
    __HAL_SD_CLEAR_FLAG(hsd, SDIO_STATIC_DATA_FLAGS);

    if (read)
    {
        g_sd_stat_start_status = sdio_send_data_cmd(
            (cnt > 1U) ? SDMMC_CMD_READ_MULT_BLOCK : SDMMC_CMD_READ_SINGLE_BLOCK,
            addr);
    }
    else
    {
        g_sd_stat_start_status = sdio_send_data_cmd(
            (cnt > 1U) ? SDMMC_CMD_WRITE_MULT_BLOCK : SDMMC_CMD_WRITE_SINGLE_BLOCK,
            addr);
    }
    if (g_sd_stat_start_status != 0U)
    {
        sdio_dma_abort();
        return 1U;
    }

    __HAL_SD_ENABLE_IT(hsd, SDIO_IT_DCRCFAIL | SDIO_IT_DTIMEOUT |
                            SDIO_IT_RXOVERR | SDIO_IT_TXUNDERR |
                            SDIO_IT_DATAEND | SDIO_IT_STBITERR);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);

    if (sdio_wait_dma() != 0U)
    {
        return 1U;
    }

    if (cnt > 1U)
    {
        (void)SDMMC_CmdStopTransfer(SDIO);
    }
    if (!read)
    {
        return (sdio_wait_card_ready(SD_TIMEOUT_COUNT) == 0U) ? 0U : 1U;
    }
    return 0U;
#endif
}

static uint8_t sdio_dma_selftest(void)
{
    static uint32_t buf[SD_BLOCK_WORDS];
    uint8_t sta;

    (void)xSemaphoreTake(g_sd_dma_sem, 0U);
    sta = sdio_transfer_dma(true, (uint8_t *)buf, 0U, 1U);
    if (sta != 0U)
    {
        g_sd_stat_start_status = sta;
        return sta;
    }
    return 0U;
}

uint8_t sdio_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

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
    g_sd_data_active = false;
    g_sd_dma_status = 0U;
    g_sd_dma_selftest_result = 0U;
    g_sd_stat_error = 0U;
    g_sd_stat_start_status = 0U;
    g_sd_stat_sdio_irq = 0U;
    g_sd_stat_dma_irq = 0U;

    if (HAL_SD_Init(&g_sdcard_handle) != HAL_OK)
    {
        return 1U;
    }

    HAL_SD_GetCardInfo(&g_sdcard_handle, &g_sd_card_info_handle);

    if (HAL_SD_ConfigWideBusOperation(&g_sdcard_handle, SDIO_BUS_WIDE_4B) != HAL_OK)
    {
        return 2U;
    }

    SDIO->MASK = 0U;
    __HAL_SD_CLEAR_FLAG(&g_sdcard_handle, SDIO_STATIC_FLAGS);
    return 0U;
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

int sdio_set_dma_mode(bool enable)
{
    if (!enable)
    {
        if (g_sd_use_dma)
        {
            (void)HAL_SD_Abort(&g_sdcard_handle);
            sdio_dma_abort();
        }
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
    uint8_t sta;

    if (!g_sd_use_dma || (((uintptr_t)pbuf & 0x3U) != 0U))
    {
        return sdio_read_blocking(pbuf, saddr, cnt);
    }

    sta = sdio_transfer_dma(true, pbuf, saddr, cnt);
    if (sta == 0U)
    {
        return 0U;
    }

    g_sd_stat_error = sta;
    g_sd_use_dma = false;
    return sdio_read_blocking(pbuf, saddr, cnt);
}

uint8_t sd_write_disk(uint8_t *pbuf, uint32_t saddr, uint32_t cnt)
{
    uint8_t sta;

    if (!g_sd_use_dma || (((uintptr_t)pbuf & 0x3U) != 0U))
    {
        return sdio_write_blocking(pbuf, saddr, cnt);
    }

    sta = sdio_transfer_dma(false, pbuf, saddr, cnt);
    if (sta == 0U)
    {
        return 0U;
    }

    g_sd_stat_error = sta;
    g_sd_use_dma = false;
    return sdio_write_blocking(pbuf, saddr, cnt);
}