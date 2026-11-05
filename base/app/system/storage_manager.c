#include <errno.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "storage_manager.h"
#include "fs.h"
#include "bsp_sd.h"
#include "mw_cherryusb.h"

static SemaphoreHandle_t g_lock;
static storage_state_t   g_state = STORAGE_STATE_UNINIT;
static volatile bool     g_initialized;

static int storage_lock(void)
{
    if (g_lock == 0)
    {
        return -1;
    }
    return (xSemaphoreTake(g_lock, portMAX_DELAY) == pdTRUE) ? 0 : -1;
}

static void storage_unlock(void)
{
    if (g_lock != 0)
    {
        (void)xSemaphoreGive(g_lock);
    }
}

int storage_init(void)
{
    if (g_initialized)
    {
        return 0;
    }
    g_lock = xSemaphoreCreateMutex();
    if (g_lock == 0)
    {
        return -1;
    }
    g_state = STORAGE_STATE_IDLE;
    g_initialized = true;
    return 0;
}

storage_state_t storage_state(void)
{
    storage_state_t state = STORAGE_STATE_UNINIT;

    if ((g_lock != 0) && (xSemaphoreTake(g_lock, portMAX_DELAY) == pdTRUE))
    {
        state = g_state;
        (void)xSemaphoreGive(g_lock);
    }
    return state;
}

const char *storage_state_name(void)
{
    switch (storage_state())
    {
        case STORAGE_STATE_UNINIT:       return "uninit";
        case STORAGE_STATE_IDLE:         return "idle";
        case STORAGE_STATE_FATFS:        return "fatfs";
        case STORAGE_STATE_MSC:          return "msc";
        case STORAGE_STATE_MSC_READONLY: return "msc_ro";
        case STORAGE_STATE_ERROR:        return "error";
        default:                         return "?";
    }
}

int storage_fatfs_mount(void)
{
    int ret;

    if (storage_lock() != 0)
    {
        return -1;
    }
    if ((g_state == STORAGE_STATE_MSC) ||
        (g_state == STORAGE_STATE_MSC_READONLY) ||
        (g_state == STORAGE_STATE_ERROR))
    {
        storage_unlock();
        return -1;
    }
    if (g_state == STORAGE_STATE_FATFS)
    {
        storage_unlock();
        return 0;
    }

    ret = fs_mount(fs_fatfs_mount_obj());
    if ((ret == 0) || (ret == -EBUSY && fs_is_mounted(FS_MOUNT_SD)))
    {
        g_state = STORAGE_STATE_FATFS;
        ret = 0;
    }
    storage_unlock();
    return ret;
}

int storage_fatfs_umount(void)
{
    int ret;

    if (storage_lock() != 0)
    {
        return -1;
    }
    if ((g_state == STORAGE_STATE_MSC) ||
        (g_state == STORAGE_STATE_MSC_READONLY))
    {
        storage_unlock();
        return -1;
    }

    ret = fs_unmount(fs_fatfs_mount_obj());
    if ((ret == 0) || (ret == -ENOENT))
    {
        g_state = STORAGE_STATE_IDLE;
    }
    storage_unlock();
    return ret;
}

int storage_msc_begin(void)
{
    int ret;

    if (storage_lock() != 0)
    {
        return -1;
    }
    if ((g_state == STORAGE_STATE_MSC) ||
        (g_state == STORAGE_STATE_MSC_READONLY))
    {
        storage_unlock();
        return 0;
    }
    if (g_state == STORAGE_STATE_ERROR)
    {
        storage_unlock();
        return -1;
    }

    if ((g_state == STORAGE_STATE_FATFS) ||
        ((g_state == STORAGE_STATE_IDLE) && fs_is_mounted(FS_MOUNT_SD)))
    {
        ret = fs_unmount(fs_fatfs_mount_obj());
        if ((ret != 0) && (ret != -ENOENT))
        {
            g_state = STORAGE_STATE_ERROR;
            storage_unlock();
            return -1;
        }
        g_state = STORAGE_STATE_IDLE;
    }

    if (bsp_sd_acquire(BSP_SD_OWNER_MSC, 1000U) != 0)
    {
        g_state = STORAGE_STATE_ERROR;
        storage_unlock();
        return -1;
    }

    g_state = bsp_sd_is_writable() ? STORAGE_STATE_MSC : STORAGE_STATE_MSC_READONLY;
    storage_unlock();
    return 0;
}

void storage_msc_end(void)
{
    if (storage_lock() == 0)
    {
        if ((g_state == STORAGE_STATE_MSC) ||
            (g_state == STORAGE_STATE_MSC_READONLY))
        {
            bsp_sd_release(BSP_SD_OWNER_MSC);
            g_state = STORAGE_STATE_IDLE;
        }
        storage_unlock();
    }
}

void storage_usb_event(uint8_t event)
{
    if ((event == MW_USB_EVENT_RESET) || (event == MW_USB_EVENT_DISCONNECTED))
    {
        storage_msc_end();
    }
}
