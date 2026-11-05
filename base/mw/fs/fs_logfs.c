#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "easyflash.h"
#include "fs.h"
#include "fs_backend.h"

#define LOGFS_FILE_PATH "/log/app.log"
#define LOGFS_WBUF_SIZE 256U

static SemaphoreHandle_t g_logfs_mutex;
static uint32_t          g_wbuf[LOGFS_WBUF_SIZE / 4U];
static uint32_t          g_wlen;
static bool              g_writer;

static int logfs_lock(void)
{
    if (g_logfs_mutex == 0)
    {
        return -1;
    }
    return (xSemaphoreTake(g_logfs_mutex, portMAX_DELAY) == pdTRUE) ? 0 : -1;
}

static void logfs_unlock(void)
{
    if (g_logfs_mutex != 0)
    {
        (void)xSemaphoreGive(g_logfs_mutex);
    }
}

static int logfs_flush_locked(void)
{
    uint32_t padded;
    EfErrCode err;

    if (g_wlen == 0U)
    {
        return 0;
    }

    padded = (g_wlen + 3U) & ~3U;
    if (padded > g_wlen)
    {
        memset((uint8_t *)g_wbuf + g_wlen, 0, padded - g_wlen);
    }
    err = ef_log_write(g_wbuf, padded);
    g_wlen = 0U;
    return (err == EF_NO_ERR) ? 0 : -EIO;
}

static int logfs_path_is_file(const char *path)
{
    return (strcmp(path, LOGFS_FILE_PATH) == 0) ||
           (strcmp(path, "/app.log") == 0);
}

static int logfs_mount(fs_mount_t *mp)
{
    (void)mp;
    if (g_logfs_mutex == 0)
    {
        g_logfs_mutex = xSemaphoreCreateMutex();
        if (g_logfs_mutex == 0)
        {
            return -ENOMEM;
        }
    }
    if (easyflash_init() != EF_NO_ERR)
    {
        return -EIO;
    }
    g_wlen = 0U;
    g_writer = false;
    return 0;
}

static int logfs_unmount(fs_mount_t *mp)
{
    (void)mp;
    if (logfs_lock() == 0)
    {
        (void)logfs_flush_locked();
        g_writer = false;
        logfs_unlock();
    }
    return 0;
}

static int logfs_open(fs_file_t *fp, const char *path, fs_mode_t flags)
{
    if (!logfs_path_is_file(path))
    {
        return -ENOENT;
    }
    if ((flags & FS_O_WRITE) != 0U)
    {
        if (logfs_lock() != 0)
        {
            return -EIO;
        }
        if (g_writer)
        {
            logfs_unlock();
            return -EBUSY;
        }
        g_writer = true;
        g_wlen = 0U;
        logfs_unlock();
    }
    fp->priv = (void *)(uintptr_t)0U;
    return 0;
}

static int logfs_close(fs_file_t *fp)
{
    if ((fp->flags & FS_O_WRITE) != 0U)
    {
        if (logfs_lock() == 0)
        {
            (void)logfs_flush_locked();
            g_writer = false;
            logfs_unlock();
        }
    }
    fp->priv = 0;
    return 0;
}

static ssize_t logfs_read(fs_file_t *fp, void *buf, size_t size)
{
    uint32_t cursor = (uint32_t)(uintptr_t)fp->priv;
    size_t   used;
    size_t   done = 0U;

    if ((buf == 0) || (size == 0U))
    {
        return 0;
    }
    if ((fp->flags & FS_O_READ) == 0U)
    {
        return -EBADF;
    }
    if (logfs_lock() != 0)
    {
        return -EIO;
    }

    used = (size_t)ef_log_get_used_size();
    while ((cursor < used) && (done < size))
    {
        uint32_t start = cursor & ~3U;
        uint32_t offset = cursor - start;
        uint32_t remain = (uint32_t)(used - cursor);
        uint32_t req = (remain > LOGFS_WBUF_SIZE) ? LOGFS_WBUF_SIZE : remain;
        uint32_t round = (req + 3U) & ~3U;
        uint32_t n = (uint32_t)(size - done);
        EfErrCode err;

        if (round > LOGFS_WBUF_SIZE)
        {
            round = LOGFS_WBUF_SIZE;
        }
        if (n > remain)
        {
            n = remain;
        }

        err = ef_log_read(start, g_wbuf, round);
        if (err != EF_NO_ERR)
        {
            logfs_unlock();
            return (done > 0U) ? (ssize_t)done : -EIO;
        }

        if (n > (round - offset))
        {
            n = round - offset;
        }
        memcpy((uint8_t *)buf + done, (const uint8_t *)g_wbuf + offset, n);
        done += n;
        cursor += n;
    }

    fp->priv = (void *)(uintptr_t)cursor;
    logfs_unlock();
    return (ssize_t)done;
}

static ssize_t logfs_write(fs_file_t *fp, const void *buf, size_t size)
{
    const uint8_t *src = (const uint8_t *)buf;
    size_t written = 0U;

    if ((buf == 0) || (size == 0U))
    {
        return 0;
    }
    if ((fp->flags & FS_O_WRITE) == 0U)
    {
        return -EBADF;
    }
    if (logfs_lock() != 0)
    {
        return -EIO;
    }

    while (written < size)
    {
        size_t space = LOGFS_WBUF_SIZE - g_wlen;
        size_t n = size - written;

        if (n > space)
        {
            n = space;
        }
        memcpy((uint8_t *)g_wbuf + g_wlen, src + written, n);
        g_wlen += (uint32_t)n;
        written += n;

        if (g_wlen == LOGFS_WBUF_SIZE)
        {
            if (ef_log_write(g_wbuf, LOGFS_WBUF_SIZE) != EF_NO_ERR)
            {
                g_wlen = 0U;
                logfs_unlock();
                return -EIO;
            }
            g_wlen = 0U;
        }
    }

    logfs_unlock();
    return (ssize_t)size;
}

static off_t logfs_seek(fs_file_t *fp, off_t off, int whence)
{
    off_t base;
    off_t target;
    off_t used;

    if (fp == 0)
    {
        return -EINVAL;
    }
    if (logfs_lock() != 0)
    {
        return -EIO;
    }
    used = (off_t)ef_log_get_used_size();
    switch (whence)
    {
        case SEEK_SET: base = 0; break;
        case SEEK_CUR: base = (off_t)(uintptr_t)fp->priv; break;
        case SEEK_END: base = used; break;
        default:
            logfs_unlock();
            return -EINVAL;
    }
    target = base + off;
    if (target < 0)
    {
        target = 0;
    }
    if (target > used)
    {
        target = used;
    }
    fp->priv = (void *)(uintptr_t)target;
    logfs_unlock();
    return target;
}

static off_t logfs_tell(fs_file_t *fp)
{
    return (fp == 0) ? -EINVAL : (off_t)(uintptr_t)fp->priv;
}

static int logfs_truncate(fs_file_t *fp, off_t length)
{
    (void)fp;
    (void)length;
    return -EROFS;
}

static int logfs_sync(fs_file_t *fp)
{
    int ret = 0;

    (void)fp;
    if (logfs_lock() == 0)
    {
        ret = logfs_flush_locked();
        logfs_unlock();
    }
    return ret;
}

static int logfs_stat(fs_mount_t *mp, const char *path, fs_dirent_t *entry)
{
    (void)mp;
    if (logfs_path_is_file(path))
    {
        if (logfs_lock() != 0)
        {
            return -EIO;
        }
        (void)strncpy(entry->name, "app.log", sizeof(entry->name) - 1U);
        entry->name[sizeof(entry->name) - 1U] = '\0';
        entry->size = (size_t)ef_log_get_used_size();
        entry->type = FS_DIR_ENTRY_FILE;
        logfs_unlock();
        return 0;
    }
    if ((strcmp(path, "/log") == 0) || (strcmp(path, "/") == 0))
    {
        (void)strncpy(entry->name, (path[1] == '\0') ? "/" : "log",
                      sizeof(entry->name) - 1U);
        entry->name[sizeof(entry->name) - 1U] = '\0';
        entry->size = 0U;
        entry->type = FS_DIR_ENTRY_DIR;
        return 0;
    }
    return -ENOENT;
}

static int logfs_unlink(fs_mount_t *mp, const char *path)
{
    (void)mp;
    if (!logfs_path_is_file(path))
    {
        return -ENOENT;
    }
    if (logfs_lock() != 0)
    {
        return -EIO;
    }
    if (g_writer)
    {
        logfs_unlock();
        return -EBUSY;
    }
    g_wlen = 0U;
    (void)ef_log_clean();
    logfs_unlock();
    return 0;
}

static int logfs_rename(fs_mount_t *mp, const char *old_path, const char *new_path)
{
    (void)mp;
    (void)old_path;
    (void)new_path;
    return -EROFS;
}

static int logfs_mkdir(fs_mount_t *mp, const char *path)
{
    (void)mp;
    return ((strcmp(path, "/log") == 0) || (strcmp(path, "/") == 0)) ? 0 : -EINVAL;
}

static int logfs_opendir(fs_dir_t *dp, const char *path)
{
    if (strcmp(path, "/") == 0)
    {
        dp->priv = (void *)(uintptr_t)0U;
        return 0;
    }
    if (strcmp(path, "/log") == 0)
    {
        dp->priv = (void *)(uintptr_t)1U;
        return 0;
    }
    return -ENOENT;
}

static int logfs_readdir(fs_dir_t *dp, fs_dirent_t *entry)
{
    uintptr_t idx = (uintptr_t)dp->priv;

    if (idx == 0U)
    {
        (void)strncpy(entry->name, "log", sizeof(entry->name) - 1U);
        entry->name[sizeof(entry->name) - 1U] = '\0';
        entry->size = 0U;
        entry->type = FS_DIR_ENTRY_DIR;
        dp->priv = (void *)(uintptr_t)3U;
        return 0;
    }
    if (idx == 1U)
    {
        if (logfs_lock() != 0)
        {
            return -EIO;
        }
        (void)strncpy(entry->name, "app.log", sizeof(entry->name) - 1U);
        entry->name[sizeof(entry->name) - 1U] = '\0';
        entry->size = (size_t)ef_log_get_used_size();
        entry->type = FS_DIR_ENTRY_FILE;
        logfs_unlock();
        dp->priv = (void *)(uintptr_t)2U;
        return 0;
    }
    return 1;
}

static int logfs_closedir(fs_dir_t *dp)
{
    (void)dp;
    return 0;
}

static int logfs_statvfs(fs_mount_t *mp, fs_statvfs_t *stat)
{
    unsigned long total;
    unsigned long used;

    (void)mp;
    if (stat == 0)
    {
        return -EINVAL;
    }
    if (logfs_lock() != 0)
    {
        return -EIO;
    }
    total = (unsigned long)ef_log_get_total_size();
    used = (unsigned long)ef_log_get_used_size();
    stat->block_size = 1U;
    stat->total_blocks = total;
    stat->free_blocks = (used < total) ? (total - used) : 0U;
    stat->total_bytes = total;
    stat->free_bytes = stat->free_blocks;
    logfs_unlock();
    return 0;
}

static const struct fs_ops g_logfs_ops =
{
    .mount = logfs_mount,
    .unmount = logfs_unmount,
    .open = logfs_open,
    .close = logfs_close,
    .read = logfs_read,
    .write = logfs_write,
    .seek = logfs_seek,
    .tell = logfs_tell,
    .truncate = logfs_truncate,
    .sync = logfs_sync,
    .stat = logfs_stat,
    .unlink = logfs_unlink,
    .rename = logfs_rename,
    .mkdir = logfs_mkdir,
    .opendir = logfs_opendir,
    .readdir = logfs_readdir,
    .closedir = logfs_closedir,
    .statvfs = logfs_statvfs,
};

static fs_mount_t g_logfs_mount =
{
    .mount_point = FS_MOUNT_FLASH,
    .ops = &g_logfs_ops,
    .ctx = 0,
    .mounted = false,
    .open_count = 0U,
};

fs_mount_t *fs_logfs_mount_obj(void)
{
    return &g_logfs_mount;
}
