#include "FreeRTOS.h"
#include "semphr.h"
#include "mw_fatfs.h"
#include "bsp_sd.h"

static FATFS             g_fs;
static SemaphoreHandle_t g_fs_mutex;
static volatile bool     g_mounted;

int mw_fatfs_init(void)
{
    if (g_fs_mutex == 0)
    {
        g_fs_mutex = xSemaphoreCreateMutex();
    }
    return (g_fs_mutex != 0) ? 0 : -1;
}

int mw_fatfs_lock(uint32_t timeout_ms)
{
    TickType_t ticks;

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        ticks = 0U;
    }
    else
    {
        ticks = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    }

    if (g_fs_mutex == 0)
    {
        return -1;
    }
    return (xSemaphoreTake(g_fs_mutex, ticks) == pdTRUE) ? 0 : -1;
}

void mw_fatfs_unlock(void)
{
    if (g_fs_mutex != 0)
    {
        (void)xSemaphoreGive(g_fs_mutex);
    }
}

int mw_fatfs_mount(void)
{
    FRESULT res;

    if (g_mounted)
    {
        return 0;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -1;
    }

    if (bsp_sd_acquire(BSP_SD_OWNER_FATFS, UINT32_MAX) != 0)
    {
        mw_fatfs_unlock();
        return -1;
    }

    res = f_mount(&g_fs, "", 1);
    if (res == FR_OK)
    {
        g_mounted = true;
    }
    else
    {
        bsp_sd_release(BSP_SD_OWNER_FATFS);
    }

    mw_fatfs_unlock();
    return (res == FR_OK) ? 0 : -1;
}

int mw_fatfs_umount(void)
{
    if (!g_mounted)
    {
        /* Still release an owner that may have been acquired but not mounted. */
        bsp_sd_release(BSP_SD_OWNER_FATFS);
        return 0;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -1;
    }

    (void)f_mount(0, "", 0);
    g_mounted = false;
    bsp_sd_release(BSP_SD_OWNER_FATFS);

    mw_fatfs_unlock();
    return 0;
}

bool mw_fatfs_is_mounted(void)
{
    return g_mounted;
}

FATFS *mw_fatfs_handle(void)
{
    return &g_fs;
}
