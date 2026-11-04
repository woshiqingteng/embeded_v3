#include "FreeRTOS.h"
#include "semphr.h"
#include "bsp_sd.h"
#include "bsp_log.h"

static SemaphoreHandle_t g_sd_mutex;
static bsp_sd_owner_t    g_owner = BSP_SD_OWNER_NONE;
static bsp_sd_io_hook_t  g_io_hook;
static uint8_t           g_ready;

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

    if (g_sd_mutex == 0)
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

    if ((g_sd_mutex == 0) ||
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

int bsp_sd_read(uint8_t *buf, uint32_t lba, uint32_t count)
{
    int ret = -1;

    if ((buf == 0) || (count == 0U))
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
    if ((buf == 0) || (count == 0U))
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
