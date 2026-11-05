#include <stddef.h>
#include <sys/types.h>

#include "fs.h"
#include "mw_log.h"
#include "bsp_log.h"

#define LOG_SYNC_BYTES   (8U * 1024U)
#define LOG_PATH         FS_MOUNT_FLASH "/log/app.log"

static fs_file_t        g_file;
static volatile int     g_open;
static uint32_t         g_since_sync;

static int log_open_append(void)
{
    int err;

    (void)fs_mkdir(FS_MOUNT_FLASH "/log");
    err = fs_open(&g_file, LOG_PATH,
                  FS_O_WRITE | FS_O_CREATE | FS_O_APPEND);
    return err;
}

static void log_sink(const uint8_t *data, uint32_t len)
{
    ssize_t written;

    if (!g_open || (data == 0) || (len == 0U))
    {
        return;
    }

    written = fs_write(&g_file, data, len);
    if (written < 0)
    {
        g_open = 0;
        (void)fs_close(&g_file);
        return;
    }

    g_since_sync += (uint32_t)written;
    if (g_since_sync >= LOG_SYNC_BYTES)
    {
        (void)fs_sync(&g_file);
        g_since_sync = 0U;
    }
}

int mw_log_fill(uint32_t bytes)
{
    static const char line[] =
        "log-fill 0123456789abcdef0123456789abcdef0123456789abcdef\r\n";
    uint32_t written = 0U;

    if (!g_open)
    {
        return -1;
    }

    while (written < bytes)
    {
        size_t n = sizeof(line) - 1U;
        ssize_t ret;

        if (n > (bytes - written))
        {
            n = bytes - written;
        }
        ret = fs_write(&g_file, line, n);
        if (ret < 0)
        {
            return -1;
        }
        written += (uint32_t)ret;
    }
    (void)fs_sync(&g_file);
    return 0;
}

int mw_log_init(void)
{
    int err;

    if (!fs_is_mounted(FS_MOUNT_FLASH))
    {
        return -1;
    }

    err = log_open_append();
    g_open = (err == 0) ? 1 : 0;
    g_since_sync = 0U;

    if (g_open)
    {
        bsp_log_attach_sink(log_sink);
    }

    return (err == 0) ? 0 : -1;
}
