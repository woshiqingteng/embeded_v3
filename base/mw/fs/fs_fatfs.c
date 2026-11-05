#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "mw_fatfs.h"
#include "bsp_sd.h"
#include "fs.h"
#include "fs_backend.h"

#define FS_FATFS_MAX_FILES 4U
#define FS_FATFS_MAX_DIRS  2U
#define FS_FATFS_PATH_MAX  256U

static FIL       g_files[FS_FATFS_MAX_FILES];
static bool      g_file_used[FS_FATFS_MAX_FILES];
static DIR       g_dirs[FS_FATFS_MAX_DIRS];
static bool      g_dir_used[FS_FATFS_MAX_DIRS];

static int fatfs_path(const char *rel, char *out, size_t out_size)
{
    size_t len;

    if ((rel == 0) || (out == 0))
    {
        return -EINVAL;
    }
    while (*rel == '/')
    {
        rel++;
    }
    len = strlen(rel);
    if (len >= out_size)
    {
        return -ENAMETOOLONG;
    }
    memcpy(out, rel, len + 1U);
    return 0;
}

static int fatfs_errno(FRESULT res)
{
    switch (res)
    {
        case FR_OK:                  return 0;
        case FR_NO_FILE:
        case FR_NO_PATH:             return -ENOENT;
        case FR_EXIST:               return -EEXIST;
        case FR_DENIED:              return -EACCES;
        case FR_INVALID_NAME:
        case FR_INVALID_PARAMETER:   return -EINVAL;
        case FR_WRITE_PROTECTED:     return -EROFS;
        case FR_NOT_ENOUGH_CORE:     return -ENOMEM;
        case FR_TIMEOUT:             return -ETIMEDOUT;
        default:                     return -EIO;
    }
}

static FIL *fatfs_alloc_file(void)
{
    for (size_t i = 0U; i < FS_FATFS_MAX_FILES; i++)
    {
        if (!g_file_used[i])
        {
            g_file_used[i] = true;
            return &g_files[i];
        }
    }
    return 0;
}

static void fatfs_free_file(FIL *fp)
{
    for (size_t i = 0U; i < FS_FATFS_MAX_FILES; i++)
    {
        if (&g_files[i] == fp)
        {
            g_file_used[i] = false;
            return;
        }
    }
}

static DIR *fatfs_alloc_dir(void)
{
    for (size_t i = 0U; i < FS_FATFS_MAX_DIRS; i++)
    {
        if (!g_dir_used[i])
        {
            g_dir_used[i] = true;
            return &g_dirs[i];
        }
    }
    return 0;
}

static void fatfs_free_dir(DIR *dp)
{
    for (size_t i = 0U; i < FS_FATFS_MAX_DIRS; i++)
    {
        if (&g_dirs[i] == dp)
        {
            g_dir_used[i] = false;
            return;
        }
    }
}

static int fatfs_mount(fs_mount_t *mp)
{
    (void)mp;
    return (mw_fatfs_mount() == 0) ? 0 : -EIO;
}

static int fatfs_unmount(fs_mount_t *mp)
{
    (void)mp;
    return (mw_fatfs_umount() == 0) ? 0 : -EIO;
}

static int fatfs_open(fs_file_t *fp, const char *path, fs_mode_t flags)
{
    char    name[FS_FATFS_PATH_MAX];
    FIL    *file;
    BYTE    mode = 0U;
    FRESULT res;

    if ((flags & (FS_O_WRITE | FS_O_CREATE | FS_O_TRUNC | FS_O_APPEND)) != 0U &&
        !mw_fatfs_is_mounted())
    {
        return -EIO;
    }
    if ((flags & (FS_O_WRITE | FS_O_CREATE | FS_O_TRUNC | FS_O_APPEND)) != 0U &&
        !bsp_sd_is_writable())
    {
        return -EROFS;
    }
    if (fatfs_path(path, name, sizeof(name)) != 0)
    {
        return -EINVAL;
    }

    if ((flags & FS_O_READ) != 0U)
    {
        mode |= FA_READ;
    }
    if ((flags & FS_O_WRITE) != 0U)
    {
        mode |= FA_WRITE;
    }
    if ((flags & FS_O_TRUNC) != 0U)
    {
        mode |= FA_CREATE_ALWAYS;
    }
    else if ((flags & FS_O_CREATE) != 0U)
    {
        mode |= FA_OPEN_ALWAYS;
    }
    if ((flags & FS_O_APPEND) != 0U)
    {
        mode |= FA_OPEN_APPEND;
    }

    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    file = fatfs_alloc_file();
    if (file == 0)
    {
        mw_fatfs_unlock();
        return -ENOMEM;
    }

    res = f_open(file, name, mode);
    if (res != FR_OK)
    {
        fatfs_free_file(file);
        mw_fatfs_unlock();
        return fatfs_errno(res);
    }

    fp->priv = file;
    mw_fatfs_unlock();
    return 0;
}

static int fatfs_close(fs_file_t *fp)
{
    FIL *file = (FIL *)fp->priv;
    FRESULT res;

    if (file == 0)
    {
        return -EINVAL;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_close(file);
    fatfs_free_file(file);
    fp->priv = 0;
    mw_fatfs_unlock();
    return fatfs_errno(res);
}

static ssize_t fatfs_read(fs_file_t *fp, void *buf, size_t size)
{
    FIL    *file = (FIL *)fp->priv;
    UINT    read = 0U;
    FRESULT res;

    if ((file == 0) || (buf == 0))
    {
        return -EINVAL;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_read(file, buf, (UINT)size, &read);
    mw_fatfs_unlock();
    return (res == FR_OK) ? (ssize_t)read : (ssize_t)fatfs_errno(res);
}

static ssize_t fatfs_write(fs_file_t *fp, const void *buf, size_t size)
{
    FIL    *file = (FIL *)fp->priv;
    UINT    written = 0U;
    FRESULT res;

    if ((file == 0) || (buf == 0))
    {
        return -EINVAL;
    }
    if (!bsp_sd_is_writable())
    {
        return -EROFS;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_write(file, buf, (UINT)size, &written);
    mw_fatfs_unlock();
    return (res == FR_OK) ? (ssize_t)written : (ssize_t)fatfs_errno(res);
}

static off_t fatfs_seek(fs_file_t *fp, off_t off, int whence)
{
    FIL     *file = (FIL *)fp->priv;
    FSIZE_t  base;
    FSIZE_t  target;
    FRESULT  res;

    if (file == 0)
    {
        return -EINVAL;
    }

    switch (whence)
    {
        case SEEK_SET: base = 0U; break;
        case SEEK_CUR: base = f_tell(file); break;
        case SEEK_END: base = f_size(file); break;
        default: return -EINVAL;
    }

    target = (FSIZE_t)((off_t)base + off);
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_lseek(file, target);
    mw_fatfs_unlock();
    return (res == FR_OK) ? (off_t)target : (off_t)fatfs_errno(res);
}

static off_t fatfs_tell(fs_file_t *fp)
{
    FIL *file = (FIL *)fp->priv;

    return (file == 0) ? -EINVAL : (off_t)f_tell(file);
}

static int fatfs_truncate(fs_file_t *fp, off_t length)
{
    FIL    *file = (FIL *)fp->priv;
    FRESULT res;

    if (file == 0)
    {
        return -EINVAL;
    }
    if (!bsp_sd_is_writable())
    {
        return -EROFS;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_lseek(file, (FSIZE_t)length);
    if (res == FR_OK)
    {
        res = f_truncate(file);
    }
    mw_fatfs_unlock();
    return fatfs_errno(res);
}

static int fatfs_sync(fs_file_t *fp)
{
    FIL    *file = (FIL *)fp->priv;
    FRESULT res;

    if (file == 0)
    {
        return -EINVAL;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_sync(file);
    mw_fatfs_unlock();
    return fatfs_errno(res);
}

static int fatfs_stat(fs_mount_t *mp, const char *path, fs_dirent_t *entry)
{
    char     name[FS_FATFS_PATH_MAX];
    FILINFO  info;
    FRESULT  res;

    (void)mp;
    if (fatfs_path(path, name, sizeof(name)) != 0)
    {
        return -EINVAL;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_stat(name, &info);
    mw_fatfs_unlock();
    if (res != FR_OK)
    {
        return fatfs_errno(res);
    }

    (void)strncpy(entry->name, info.fname, sizeof(entry->name) - 1U);
    entry->name[sizeof(entry->name) - 1U] = '\0';
    entry->size = (size_t)info.fsize;
    entry->type = ((info.fattrib & AM_DIR) != 0U) ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE;
    return 0;
}

static int fatfs_unlink(fs_mount_t *mp, const char *path)
{
    char    name[FS_FATFS_PATH_MAX];
    FRESULT res;

    (void)mp;
    if (!bsp_sd_is_writable())
    {
        return -EROFS;
    }
    if (fatfs_path(path, name, sizeof(name)) != 0)
    {
        return -EINVAL;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_unlink(name);
    mw_fatfs_unlock();
    return fatfs_errno(res);
}

static int fatfs_rename(fs_mount_t *mp, const char *old_path, const char *new_path)
{
    char    old_name[FS_FATFS_PATH_MAX];
    char    new_name[FS_FATFS_PATH_MAX];
    FRESULT res;

    (void)mp;
    if (!bsp_sd_is_writable())
    {
        return -EROFS;
    }
    if ((fatfs_path(old_path, old_name, sizeof(old_name)) != 0) ||
        (fatfs_path(new_path, new_name, sizeof(new_name)) != 0))
    {
        return -EINVAL;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_rename(old_name, new_name);
    mw_fatfs_unlock();
    return fatfs_errno(res);
}

static int fatfs_mkdir(fs_mount_t *mp, const char *path)
{
    char    name[FS_FATFS_PATH_MAX];
    FRESULT res;

    (void)mp;
    if (!bsp_sd_is_writable())
    {
        return -EROFS;
    }
    if (fatfs_path(path, name, sizeof(name)) != 0)
    {
        return -EINVAL;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_mkdir(name);
    mw_fatfs_unlock();
    return fatfs_errno(res);
}

static int fatfs_opendir(fs_dir_t *dp, const char *path)
{
    char    name[FS_FATFS_PATH_MAX];
    DIR    *dir;
    FRESULT res;

    if (fatfs_path(path, name, sizeof(name)) != 0)
    {
        return -EINVAL;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    dir = fatfs_alloc_dir();
    if (dir == 0)
    {
        mw_fatfs_unlock();
        return -ENOMEM;
    }

    res = f_opendir(dir, name);
    if (res != FR_OK)
    {
        fatfs_free_dir(dir);
        mw_fatfs_unlock();
        return fatfs_errno(res);
    }
    dp->priv = dir;
    mw_fatfs_unlock();
    return 0;
}

static int fatfs_readdir(fs_dir_t *dp, fs_dirent_t *entry)
{
    DIR    *dir = (DIR *)dp->priv;
    FILINFO info;
    FRESULT res;

    if (dir == 0)
    {
        return -EINVAL;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    memset(&info, 0, sizeof(info));
    res = f_readdir(dir, &info);
    mw_fatfs_unlock();
    if (res != FR_OK)
    {
        return fatfs_errno(res);
    }
    if (info.fname[0] == '\0')
    {
        return 1;
    }

    (void)strncpy(entry->name, info.fname, sizeof(entry->name) - 1U);
    entry->name[sizeof(entry->name) - 1U] = '\0';
    entry->size = (size_t)info.fsize;
    entry->type = ((info.fattrib & AM_DIR) != 0U) ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE;
    return 0;
}

static int fatfs_closedir(fs_dir_t *dp)
{
    DIR    *dir = (DIR *)dp->priv;
    FRESULT res;

    if (dir == 0)
    {
        return -EINVAL;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_closedir(dir);
    fatfs_free_dir(dir);
    dp->priv = 0;
    mw_fatfs_unlock();
    return fatfs_errno(res);
}

static int fatfs_statvfs(fs_mount_t *mp, fs_statvfs_t *stat)
{
    FATFS *fs;
    DWORD  free_clusters = 0U;
    DWORD  total_clusters;
    DWORD  cluster_bytes;
    FRESULT res;

    (void)mp;
    if (stat == 0)
    {
        return -EINVAL;
    }
    if (mw_fatfs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    res = f_getfree("", &free_clusters, &fs);
    mw_fatfs_unlock();
    if (res != FR_OK)
    {
        return fatfs_errno(res);
    }

    total_clusters = fs->n_fatent - 2U;
    cluster_bytes = (DWORD)fs->csize * 512U;

    stat->block_size = cluster_bytes;
    stat->total_blocks = total_clusters;
    stat->free_blocks = free_clusters;
    stat->total_bytes = (unsigned long)total_clusters * cluster_bytes;
    stat->free_bytes = (unsigned long)free_clusters * cluster_bytes;
    return 0;
}

static const struct fs_ops g_fatfs_ops =
{
    .mount = fatfs_mount,
    .unmount = fatfs_unmount,
    .open = fatfs_open,
    .close = fatfs_close,
    .read = fatfs_read,
    .write = fatfs_write,
    .seek = fatfs_seek,
    .tell = fatfs_tell,
    .truncate = fatfs_truncate,
    .sync = fatfs_sync,
    .stat = fatfs_stat,
    .unlink = fatfs_unlink,
    .rename = fatfs_rename,
    .mkdir = fatfs_mkdir,
    .opendir = fatfs_opendir,
    .readdir = fatfs_readdir,
    .closedir = fatfs_closedir,
    .statvfs = fatfs_statvfs,
};

static fs_mount_t g_fatfs_mount =
{
    .mount_point = FS_MOUNT_SD,
    .ops = &g_fatfs_ops,
    .ctx = 0,
    .mounted = false,
    .open_count = 0U,
};

fs_mount_t *fs_fatfs_mount_obj(void)
{
    return &g_fatfs_mount;
}
