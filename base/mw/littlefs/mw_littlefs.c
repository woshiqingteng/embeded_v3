#include "FreeRTOS.h"
#include "semphr.h"
#include "mw_littlefs.h"
#include "lfs_port.h"

static lfs_t             g_lfs;
static SemaphoreHandle_t g_lfs_mutex;
static volatile bool     g_mounted;

int mw_littlefs_init(void)
{
    if (g_lfs_mutex == 0)
    {
        g_lfs_mutex = xSemaphoreCreateMutex();
    }
    lfs_port_init();
    return (g_lfs_mutex != 0) ? 0 : -1;
}

int mw_littlefs_lock(uint32_t timeout_ms)
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

    if (g_lfs_mutex == 0)
    {
        return -1;
    }
    return (xSemaphoreTake(g_lfs_mutex, ticks) == pdTRUE) ? 0 : -1;
}

void mw_littlefs_unlock(void)
{
    if (g_lfs_mutex != 0)
    {
        (void)xSemaphoreGive(g_lfs_mutex);
    }
}

int mw_littlefs_mount(void)
{
    int ret;

    if (g_mounted)
    {
        return 0;
    }
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -1;
    }

    ret = lfs_mount(&g_lfs, lfs_port_config());
    g_mounted = (ret == LFS_ERR_OK);

    mw_littlefs_unlock();
    return ret;
}

int mw_littlefs_umount(void)
{
    int ret;

    if (!g_mounted)
    {
        return 0;
    }
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -1;
    }

    ret = lfs_unmount(&g_lfs);
    if (ret == LFS_ERR_OK)
    {
        g_mounted = false;
    }

    mw_littlefs_unlock();
    return ret;
}

int mw_littlefs_format(void)
{
    int ret;

    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -1;
    }
    ret = lfs_format(&g_lfs, lfs_port_config());
    mw_littlefs_unlock();
    g_mounted = false;
    return ret;
}

bool mw_littlefs_is_mounted(void)
{
    return g_mounted;
}

lfs_t *mw_littlefs_handle(void)
{
    return &g_lfs;
}
