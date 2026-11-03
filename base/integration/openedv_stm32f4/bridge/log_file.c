#include <stddef.h>

#include "lfs.h"
#include "lfs_config.h"
#include "mw_littlefs.h"
#include "mw_log.h"
#include "bsp_log.h"

#define LOG_ROTATE_BYTES (256U * 1024U)
#define LOG_SYNC_BYTES   (8U * 1024U)
#define LOG_ROTATE_PATH  "/log/app.1.log"

static lfs_file_t g_file;
static volatile int g_open;
static uint32_t g_size;
static uint32_t g_since_sync;

static int log_open_append(void)
{
    lfs_t *lfs = mw_littlefs_handle();
    int err;

    (void)lfs_mkdir(lfs, "/log");
    err = lfs_file_open(lfs, &g_file, LFS_LOG_PATH,
                        LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND);
    if (err == LFS_ERR_OK)
    {
        g_size = (uint32_t)lfs_file_size(lfs, &g_file);
    }
    return err;
}

static void log_rotate(void)
{
    lfs_t *lfs = mw_littlefs_handle();

    (void)lfs_file_close(lfs, &g_file);
    (void)lfs_remove(lfs, LOG_ROTATE_PATH);
    (void)lfs_rename(lfs, LFS_LOG_PATH, LOG_ROTATE_PATH);
    if (log_open_append() != LFS_ERR_OK)
    {
        g_open = 0;
    }
    else
    {
        g_size = 0U;
    }
}

static void log_sink(const uint8_t *data, uint32_t len)
{
    lfs_t *lfs;
    lfs_ssize_t written;

    if (!g_open || (data == 0) || (len == 0U))
    {
        return;
    }
    if (mw_littlefs_lock(100U) != 0)
    {
        return;
    }

    lfs = mw_littlefs_handle();
    written = lfs_file_write(lfs, &g_file, data, len);
    if (written < 0)
    {
        g_open = 0;
        mw_littlefs_unlock();
        return;
    }

    g_size += (uint32_t)written;
    g_since_sync += (uint32_t)written;

    if (g_since_sync >= LOG_SYNC_BYTES)
    {
        (void)lfs_file_sync(lfs, &g_file);
        g_since_sync = 0U;
    }

    if (g_size >= LOG_ROTATE_BYTES)
    {
        log_rotate();
    }

    mw_littlefs_unlock();
}

int mw_log_init(void)
{
    int err;

    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -1;
    }

    err = log_open_append();
    g_open = (err == LFS_ERR_OK) ? 1 : 0;
    g_since_sync = 0U;
    mw_littlefs_unlock();

    if (g_open)
    {
        bsp_log_attach_sink(log_sink);
    }

    return (err == LFS_ERR_OK) ? 0 : -1;
}

void mw_log_shutdown(void)
{
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return;
    }
    if (g_open)
    {
        (void)lfs_file_close(mw_littlefs_handle(), &g_file);
        g_open = 0;
    }
    mw_littlefs_unlock();
    bsp_log_attach_sink(0);
}
