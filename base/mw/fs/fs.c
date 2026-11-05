#include <errno.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "fs.h"
#include "fs_backend.h"
#include "mw_fatfs.h"
#include "mw_littlefs.h"

#define FS_MAX_MOUNTS 3U

static fs_mount_t        *g_mounts[FS_MAX_MOUNTS];
static size_t             g_mount_count;
static SemaphoreHandle_t  g_fs_mutex;
static bool               g_fs_ready;

static int fs_lock(void)
{
    if (g_fs_mutex == 0)
    {
        return -1;
    }
    return (xSemaphoreTake(g_fs_mutex, portMAX_DELAY) == pdTRUE) ? 0 : -1;
}

static void fs_unlock(void)
{
    if (g_fs_mutex != 0)
    {
        (void)xSemaphoreGive(g_fs_mutex);
    }
}

int fs_init(void)
{
    if (g_fs_ready)
    {
        return 0;
    }
    g_fs_mutex = xSemaphoreCreateMutex();
    if (g_fs_mutex == 0)
    {
        return -1;
    }
    g_mount_count = 0U;
    if (mw_fatfs_init() != 0)
    {
        return -EIO;
    }
    if (mw_littlefs_init() != 0)
    {
        return -EIO;
    }
    g_fs_ready = true;
    return 0;
}

static fs_mount_t *fs_lookup_locked(const char *path, const char **rel)
{
    fs_mount_t *best = 0;
    size_t      best_len = 0U;

    if (path == 0)
    {
        return 0;
    }

    for (size_t i = 0U; i < g_mount_count; i++)
    {
        fs_mount_t *mp = g_mounts[i];
        size_t      len;

        if ((mp == 0) || (mp->mount_point == 0))
        {
            continue;
        }

        len = strlen(mp->mount_point);
        if ((len <= best_len) || (strncmp(path, mp->mount_point, len) != 0))
        {
            continue;
        }
        if ((path[len] != '\0') && (path[len] != '/'))
        {
            continue;
        }

        best = mp;
        best_len = len;
        if (rel != 0)
        {
            *rel = (path[len] == '\0') ? "/" : &path[len];
        }
    }

    return best;
}

static fs_mount_t *fs_ref_acquire(const char *path, const char **rel)
{
    fs_mount_t *mp = 0;

    if (fs_lock() != 0)
    {
        return 0;
    }
    mp = fs_lookup_locked(path, rel);
    if ((mp != 0) && (mp->state == FS_STATE_MOUNTED))
    {
        mp->open_count++;
    }
    else
    {
        mp = 0;
    }
    fs_unlock();
    return mp;
}

static void fs_ref_release(fs_mount_t *mp)
{
    if ((mp == 0) || (fs_lock() != 0))
    {
        return;
    }
    if (mp->open_count > 0U)
    {
        mp->open_count--;
    }
    fs_unlock();
}

int fs_mount(fs_mount_t *mp)
{
    int ret;

    if ((mp == 0) || (mp->ops == 0) || (mp->ops->mount == 0) ||
        (mp->mount_point == 0))
    {
        return -EINVAL;
    }
    if ((g_fs_mutex == 0) && (fs_init() != 0))
    {
        return -EIO;
    }

    if (fs_lock() != 0)
    {
        return -EIO;
    }
    if (g_mount_count >= FS_MAX_MOUNTS)
    {
        fs_unlock();
        return -ENOMEM;
    }
    if ((mp->state != FS_STATE_UNMOUNTED) ||
        (fs_lookup_locked(mp->mount_point, 0) != 0))
    {
        fs_unlock();
        return -EBUSY;
    }
    mp->state = FS_STATE_MOUNTING;
    fs_unlock();

    ret = mp->ops->mount(mp);

    if (fs_lock() != 0)
    {
        if (ret == 0)
        {
            (void)mp->ops->unmount(mp);
        }
        mp->state = FS_STATE_UNMOUNTED;
        mp->mounted = false;
        return -EIO;
    }
    if (ret != 0)
    {
        mp->state = FS_STATE_UNMOUNTED;
        fs_unlock();
        return ret;
    }

    mp->state = FS_STATE_MOUNTED;
    mp->mounted = true;
    mp->open_count = 0U;
    g_mounts[g_mount_count++] = mp;
    fs_unlock();
    return 0;
}

int fs_unmount(fs_mount_t *mp)
{
    int ret;

    if ((mp == 0) || (mp->ops == 0) || (mp->ops->unmount == 0))
    {
        return -EINVAL;
    }
    if (fs_lock() != 0)
    {
        return -EIO;
    }
    if (mp->state == FS_STATE_UNMOUNTED)
    {
        fs_unlock();
        return 0;
    }
    if (mp->state != FS_STATE_MOUNTED)
    {
        fs_unlock();
        return -EBUSY;
    }
    if (mp->open_count != 0U)
    {
        fs_unlock();
        return -EBUSY;
    }
    mp->state = FS_STATE_UNMOUNTING;
    fs_unlock();

    ret = mp->ops->unmount(mp);

    if (fs_lock() != 0)
    {
        return -EIO;
    }
    if (ret != 0)
    {
        mp->state = FS_STATE_MOUNTED;
        fs_unlock();
        return ret;
    }

    for (size_t i = 0U; i < g_mount_count; i++)
    {
        if (g_mounts[i] == mp)
        {
            for (size_t j = i + 1U; j < g_mount_count; j++)
            {
                g_mounts[j - 1U] = g_mounts[j];
            }
            g_mount_count--;
            break;
        }
    }
    mp->state = FS_STATE_UNMOUNTED;
    mp->mounted = false;
    mp->open_count = 0U;
    fs_unlock();
    return 0;
}

bool fs_is_mounted(const char *path)
{
    fs_mount_t *mp;
    bool        mounted = false;

    if (fs_lock() != 0)
    {
        return false;
    }
    mp = fs_lookup_locked(path, 0);
    mounted = (mp != 0) && (mp->state == FS_STATE_MOUNTED);
    fs_unlock();
    return mounted;
}

int fs_open(fs_file_t *fp, const char *path, fs_mode_t flags)
{
    const char *rel = 0;
    fs_mount_t *mp;
    int         ret;

    if ((fp == 0) || (path == 0))
    {
        return -EINVAL;
    }
    mp = fs_ref_acquire(path, &rel);
    if (mp == 0)
    {
        return -ENOENT;
    }

    fp->mp = mp;
    fp->priv = 0;
    fp->flags = flags;
    ret = mp->ops->open(fp, rel, flags);
    if (ret != 0)
    {
        fs_ref_release(mp);
        fp->mp = 0;
    }
    return ret;
}

int fs_close(fs_file_t *fp)
{
    int ret;

    if ((fp == 0) || (fp->mp == 0) || (fp->mp->ops->close == 0))
    {
        return -EINVAL;
    }
    ret = fp->mp->ops->close(fp);
    fs_ref_release(fp->mp);
    fp->mp = 0;
    fp->priv = 0;
    return ret;
}

ssize_t fs_read(fs_file_t *fp, void *buf, size_t size)
{
    if ((fp == 0) || (fp->mp == 0) || (fp->mp->ops->read == 0))
    {
        return -EINVAL;
    }
    return fp->mp->ops->read(fp, buf, size);
}

ssize_t fs_write(fs_file_t *fp, const void *buf, size_t size)
{
    if ((fp == 0) || (fp->mp == 0) || (fp->mp->ops->write == 0))
    {
        return -EINVAL;
    }
    return fp->mp->ops->write(fp, buf, size);
}

off_t fs_seek(fs_file_t *fp, off_t off, int whence)
{
    if ((fp == 0) || (fp->mp == 0) || (fp->mp->ops->seek == 0))
    {
        return -EINVAL;
    }
    return fp->mp->ops->seek(fp, off, whence);
}

off_t fs_tell(fs_file_t *fp)
{
    if ((fp == 0) || (fp->mp == 0) || (fp->mp->ops->tell == 0))
    {
        return -EINVAL;
    }
    return fp->mp->ops->tell(fp);
}

int fs_truncate(fs_file_t *fp, off_t length)
{
    if ((fp == 0) || (fp->mp == 0) || (fp->mp->ops->truncate == 0))
    {
        return -EINVAL;
    }
    return fp->mp->ops->truncate(fp, length);
}

int fs_sync(fs_file_t *fp)
{
    if ((fp == 0) || (fp->mp == 0) || (fp->mp->ops->sync == 0))
    {
        return -EINVAL;
    }
    return fp->mp->ops->sync(fp);
}

int fs_stat(const char *path, fs_dirent_t *entry)
{
    const char *rel = 0;
    fs_mount_t *mp;
    int         ret;

    if (entry == 0)
    {
        return -EINVAL;
    }
    mp = fs_ref_acquire(path, &rel);
    if (mp == 0)
    {
        return -ENOENT;
    }
    if (mp->ops->stat == 0)
    {
        fs_ref_release(mp);
        return -ENOSYS;
    }
    ret = mp->ops->stat(mp, rel, entry);
    fs_ref_release(mp);
    return ret;
}

int fs_unlink(const char *path)
{
    const char *rel = 0;
    fs_mount_t *mp;
    int         ret;

    mp = fs_ref_acquire(path, &rel);
    if (mp == 0)
    {
        return -ENOENT;
    }
    if (mp->ops->unlink == 0)
    {
        fs_ref_release(mp);
        return -ENOSYS;
    }
    ret = mp->ops->unlink(mp, rel);
    fs_ref_release(mp);
    return ret;
}

int fs_rename(const char *old_path, const char *new_path)
{
    const char *old_rel = 0;
    const char *new_rel = 0;
    fs_mount_t *old_mp;
    fs_mount_t *new_mp;
    int         ret;

    if ((old_path == 0) || (new_path == 0))
    {
        return -EINVAL;
    }
    if (fs_lock() != 0)
    {
        return -EIO;
    }
    old_mp = fs_lookup_locked(old_path, &old_rel);
    new_mp = fs_lookup_locked(new_path, &new_rel);
    if ((old_mp == 0) || (new_mp == 0) ||
        (old_mp->state != FS_STATE_MOUNTED) ||
        (new_mp->state != FS_STATE_MOUNTED))
    {
        fs_unlock();
        return -ENOENT;
    }
    if (old_mp != new_mp)
    {
        fs_unlock();
        return -EXDEV;
    }
    if (old_mp->ops->rename == 0)
    {
        fs_unlock();
        return -ENOSYS;
    }
    old_mp->open_count++;
    fs_unlock();

    ret = old_mp->ops->rename(old_mp, old_rel, new_rel);
    fs_ref_release(old_mp);
    return ret;
}

int fs_mkdir(const char *path)
{
    const char *rel = 0;
    fs_mount_t *mp;
    int         ret;

    mp = fs_ref_acquire(path, &rel);
    if (mp == 0)
    {
        return -ENOENT;
    }
    if (mp->ops->mkdir == 0)
    {
        fs_ref_release(mp);
        return -ENOSYS;
    }
    ret = mp->ops->mkdir(mp, rel);
    fs_ref_release(mp);
    return ret;
}

int fs_opendir(fs_dir_t *dp, const char *path)
{
    const char *rel = 0;
    fs_mount_t *mp;
    int         ret;

    if ((dp == 0) || (path == 0))
    {
        return -EINVAL;
    }
    mp = fs_ref_acquire(path, &rel);
    if (mp == 0)
    {
        return -ENOENT;
    }
    if (mp->ops->opendir == 0)
    {
        fs_ref_release(mp);
        return -ENOSYS;
    }

    dp->mp = mp;
    dp->priv = 0;
    ret = mp->ops->opendir(dp, rel);
    if (ret != 0)
    {
        fs_ref_release(mp);
        dp->mp = 0;
    }
    return ret;
}

int fs_readdir(fs_dir_t *dp, fs_dirent_t *entry)
{
    if ((dp == 0) || (dp->mp == 0) || (entry == 0) ||
        (dp->mp->ops->readdir == 0))
    {
        return -EINVAL;
    }
    return dp->mp->ops->readdir(dp, entry);
}

int fs_closedir(fs_dir_t *dp)
{
    int ret;

    if ((dp == 0) || (dp->mp == 0) || (dp->mp->ops->closedir == 0))
    {
        return -EINVAL;
    }
    ret = dp->mp->ops->closedir(dp);
    fs_ref_release(dp->mp);
    dp->mp = 0;
    dp->priv = 0;
    return ret;
}

int fs_statvfs(const char *path, fs_statvfs_t *stat)
{
    const char *rel = 0;
    fs_mount_t *mp;
    int         ret;

    if (stat == 0)
    {
        return -EINVAL;
    }
    mp = fs_ref_acquire(path, &rel);
    if (mp == 0)
    {
        return -ENOENT;
    }
    if (mp->ops->statvfs == 0)
    {
        fs_ref_release(mp);
        return -ENOSYS;
    }
    ret = mp->ops->statvfs(mp, stat);
    fs_ref_release(mp);
    return ret;
}
