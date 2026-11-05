#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "lfs.h"
#include "lfs_config.h"
#include "mw_littlefs.h"
#include "fs.h"
#include "fs_backend.h"

#define FS_LFS_MAX_FILES 4U
#define FS_LFS_MAX_DIRS  2U

static lfs_file_t g_files[FS_LFS_MAX_FILES];
static bool       g_file_used[FS_LFS_MAX_FILES];
static lfs_dir_t  g_dirs[FS_LFS_MAX_DIRS];
static bool       g_dir_used[FS_LFS_MAX_DIRS];

static int lfs_errno(int err)
{
    switch (err)
    {
        case LFS_ERR_OK:          return 0;
        case LFS_ERR_IO:          return -EIO;
        case LFS_ERR_CORRUPT:     return -EIO;
        case LFS_ERR_NOENT:       return -ENOENT;
        case LFS_ERR_EXIST:       return -EEXIST;
        case LFS_ERR_NOTDIR:      return -ENOTDIR;
        case LFS_ERR_ISDIR:       return -EISDIR;
        case LFS_ERR_NOTEMPTY:    return -ENOTEMPTY;
        case LFS_ERR_BADF:        return -EBADF;
        case LFS_ERR_NOMEM:       return -ENOMEM;
        case LFS_ERR_NAMETOOLONG: return -ENAMETOOLONG;
        case LFS_ERR_INVAL:       return -EINVAL;
        case LFS_ERR_FBIG:        return -EFBIG;
        case LFS_ERR_NOSPC:       return -ENOSPC;
        default:                  return -EIO;
    }
}

static lfs_file_t *lfs_alloc_file(void)
{
    for (size_t i = 0U; i < FS_LFS_MAX_FILES; i++)
    {
        if (!g_file_used[i])
        {
            g_file_used[i] = true;
            return &g_files[i];
        }
    }
    return 0;
}

static void lfs_free_file(lfs_file_t *fp)
{
    for (size_t i = 0U; i < FS_LFS_MAX_FILES; i++)
    {
        if (&g_files[i] == fp)
        {
            g_file_used[i] = false;
            return;
        }
    }
}

static lfs_dir_t *lfs_alloc_dir(void)
{
    for (size_t i = 0U; i < FS_LFS_MAX_DIRS; i++)
    {
        if (!g_dir_used[i])
        {
            g_dir_used[i] = true;
            return &g_dirs[i];
        }
    }
    return 0;
}

static void lfs_free_dir(lfs_dir_t *dp)
{
    for (size_t i = 0U; i < FS_LFS_MAX_DIRS; i++)
    {
        if (&g_dirs[i] == dp)
        {
            g_dir_used[i] = false;
            return;
        }
    }
}

static int littlefs_mount(fs_mount_t *mp)
{
    (void)mp;
    return (mw_littlefs_mount() == 0) ? 0 : -EIO;
}

static int littlefs_unmount(fs_mount_t *mp)
{
    (void)mp;
    return (mw_littlefs_umount() == 0) ? 0 : -EIO;
}

static int littlefs_open(fs_file_t *fp, const char *path, fs_mode_t flags)
{
    lfs_file_t *file;
    int         oflags = 0;
    int         err;

    if ((flags & FS_O_READ) && (flags & FS_O_WRITE))
    {
        oflags |= LFS_O_RDWR;
    }
    else if ((flags & FS_O_WRITE) != 0U)
    {
        oflags |= LFS_O_WRONLY;
    }
    else
    {
        oflags |= LFS_O_RDONLY;
    }
    if ((flags & FS_O_CREATE) != 0U)
    {
        oflags |= LFS_O_CREAT;
    }
    if ((flags & FS_O_APPEND) != 0U)
    {
        oflags |= LFS_O_APPEND;
    }
    if ((flags & FS_O_TRUNC) != 0U)
    {
        oflags |= LFS_O_TRUNC;
    }

    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    file = lfs_alloc_file();
    if (file == 0)
    {
        mw_littlefs_unlock();
        return -ENOMEM;
    }

    err = lfs_file_open(mw_littlefs_handle(), file, path, oflags);
    if (err != LFS_ERR_OK)
    {
        lfs_free_file(file);
        mw_littlefs_unlock();
        return lfs_errno(err);
    }

    fp->priv = file;
    mw_littlefs_unlock();
    return 0;
}

static int littlefs_close(fs_file_t *fp)
{
    lfs_file_t *file = (lfs_file_t *)fp->priv;
    int         err;

    if (file == 0)
    {
        return -EINVAL;
    }
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    err = lfs_file_close(mw_littlefs_handle(), file);
    lfs_free_file(file);
    fp->priv = 0;
    mw_littlefs_unlock();
    return lfs_errno(err);
}

static ssize_t littlefs_read(fs_file_t *fp, void *buf, size_t size)
{
    lfs_file_t *file = (lfs_file_t *)fp->priv;
    lfs_ssize_t n;

    if ((file == 0) || (buf == 0))
    {
        return -EINVAL;
    }
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    n = lfs_file_read(mw_littlefs_handle(), file, buf, size);
    mw_littlefs_unlock();
    return (n < 0) ? (ssize_t)lfs_errno((int)n) : (ssize_t)n;
}

static ssize_t littlefs_write(fs_file_t *fp, const void *buf, size_t size)
{
    lfs_file_t *file = (lfs_file_t *)fp->priv;
    lfs_ssize_t n;

    if ((file == 0) || (buf == 0))
    {
        return -EINVAL;
    }
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    n = lfs_file_write(mw_littlefs_handle(), file, buf, size);
    mw_littlefs_unlock();
    return (n < 0) ? (ssize_t)lfs_errno((int)n) : (ssize_t)n;
}

static off_t littlefs_seek(fs_file_t *fp, off_t off, int whence)
{
    lfs_file_t *file = (lfs_file_t *)fp->priv;
    int         lfs_whence;
    lfs_soff_t  pos;

    if (file == 0)
    {
        return -EINVAL;
    }
    switch (whence)
    {
        case SEEK_SET: lfs_whence = LFS_SEEK_SET; break;
        case SEEK_CUR: lfs_whence = LFS_SEEK_CUR; break;
        case SEEK_END: lfs_whence = LFS_SEEK_END; break;
        default: return -EINVAL;
    }

    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    pos = lfs_file_seek(mw_littlefs_handle(), file, off, lfs_whence);
    mw_littlefs_unlock();
    return (pos < 0) ? (off_t)lfs_errno((int)pos) : (off_t)pos;
}

static off_t littlefs_tell(fs_file_t *fp)
{
    lfs_file_t *file = (lfs_file_t *)fp->priv;
    lfs_soff_t  pos;

    if (file == 0)
    {
        return -EINVAL;
    }
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    pos = lfs_file_tell(mw_littlefs_handle(), file);
    mw_littlefs_unlock();
    return (pos < 0) ? (off_t)lfs_errno((int)pos) : (off_t)pos;
}

static int littlefs_truncate(fs_file_t *fp, off_t length)
{
    lfs_file_t *file = (lfs_file_t *)fp->priv;
    int         err;

    if (file == 0)
    {
        return -EINVAL;
    }
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    err = lfs_file_truncate(mw_littlefs_handle(), file, (lfs_off_t)length);
    mw_littlefs_unlock();
    return lfs_errno(err);
}

static int littlefs_sync(fs_file_t *fp)
{
    lfs_file_t *file = (lfs_file_t *)fp->priv;
    int         err;

    if (file == 0)
    {
        return -EINVAL;
    }
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    err = lfs_file_sync(mw_littlefs_handle(), file);
    mw_littlefs_unlock();
    return lfs_errno(err);
}

static int littlefs_stat(fs_mount_t *mp, const char *path, fs_dirent_t *entry)
{
    struct lfs_info info;
    int             err;

    (void)mp;
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    err = lfs_stat(mw_littlefs_handle(), path, &info);
    mw_littlefs_unlock();
    if (err != LFS_ERR_OK)
    {
        return lfs_errno(err);
    }

    (void)strncpy(entry->name, info.name, sizeof(entry->name) - 1U);
    entry->name[sizeof(entry->name) - 1U] = '\0';
    entry->size = (size_t)info.size;
    entry->type = (info.type == LFS_TYPE_DIR) ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE;
    return 0;
}

static int littlefs_unlink(fs_mount_t *mp, const char *path)
{
    int err;

    (void)mp;
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    err = lfs_remove(mw_littlefs_handle(), path);
    mw_littlefs_unlock();
    return lfs_errno(err);
}

static int littlefs_rename(fs_mount_t *mp, const char *old_path, const char *new_path)
{
    int err;

    (void)mp;
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    err = lfs_rename(mw_littlefs_handle(), old_path, new_path);
    mw_littlefs_unlock();
    return lfs_errno(err);
}

static int littlefs_mkdir(fs_mount_t *mp, const char *path)
{
    int err;

    (void)mp;
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    err = lfs_mkdir(mw_littlefs_handle(), path);
    mw_littlefs_unlock();
    return lfs_errno(err);
}

static int littlefs_opendir(fs_dir_t *dp, const char *path)
{
    lfs_dir_t *dir;
    int        err;

    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    dir = lfs_alloc_dir();
    if (dir == 0)
    {
        mw_littlefs_unlock();
        return -ENOMEM;
    }

    err = lfs_dir_open(mw_littlefs_handle(), dir, path);
    if (err != LFS_ERR_OK)
    {
        lfs_free_dir(dir);
        mw_littlefs_unlock();
        return lfs_errno(err);
    }
    dp->priv = dir;
    mw_littlefs_unlock();
    return 0;
}

static int littlefs_readdir(fs_dir_t *dp, fs_dirent_t *entry)
{
    lfs_dir_t      *dir = (lfs_dir_t *)dp->priv;
    struct lfs_info info;
    int             err;

    if (dir == 0)
    {
        return -EINVAL;
    }
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    err = lfs_dir_read(mw_littlefs_handle(), dir, &info);
    mw_littlefs_unlock();
    if (err < 0)
    {
        return lfs_errno(err);
    }
    if (err == 0)
    {
        return 1;
    }

    (void)strncpy(entry->name, info.name, sizeof(entry->name) - 1U);
    entry->name[sizeof(entry->name) - 1U] = '\0';
    entry->size = (size_t)info.size;
    entry->type = (info.type == LFS_TYPE_DIR) ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE;
    return 0;
}

static int littlefs_closedir(fs_dir_t *dp)
{
    lfs_dir_t *dir = (lfs_dir_t *)dp->priv;
    int        err;

    if (dir == 0)
    {
        return -EINVAL;
    }
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    err = lfs_dir_close(mw_littlefs_handle(), dir);
    lfs_free_dir(dir);
    dp->priv = 0;
    mw_littlefs_unlock();
    return lfs_errno(err);
}

static int littlefs_statvfs(fs_mount_t *mp, fs_statvfs_t *stat)
{
    lfs_ssize_t used;
    unsigned long total = LFS_BLOCK_COUNT;
    unsigned long used_blocks;

    (void)mp;
    if (stat == 0)
    {
        return -EINVAL;
    }
    if (mw_littlefs_lock(UINT32_MAX) != 0)
    {
        return -EIO;
    }
    used = lfs_fs_size(mw_littlefs_handle());
    mw_littlefs_unlock();
    if (used < 0)
    {
        return lfs_errno((int)used);
    }

    used_blocks = (unsigned long)used;
    if (used_blocks > total)
    {
        used_blocks = total;
    }

    stat->block_size = LFS_BLOCK_SIZE;
    stat->total_blocks = total;
    stat->free_blocks = total - used_blocks;
    stat->total_bytes = total * LFS_BLOCK_SIZE;
    stat->free_bytes = stat->free_blocks * LFS_BLOCK_SIZE;
    return 0;
}

static const struct fs_ops g_littlefs_ops =
{
    .mount = littlefs_mount,
    .unmount = littlefs_unmount,
    .open = littlefs_open,
    .close = littlefs_close,
    .read = littlefs_read,
    .write = littlefs_write,
    .seek = littlefs_seek,
    .tell = littlefs_tell,
    .truncate = littlefs_truncate,
    .sync = littlefs_sync,
    .stat = littlefs_stat,
    .unlink = littlefs_unlink,
    .rename = littlefs_rename,
    .mkdir = littlefs_mkdir,
    .opendir = littlefs_opendir,
    .readdir = littlefs_readdir,
    .closedir = littlefs_closedir,
    .statvfs = littlefs_statvfs,
};

static fs_mount_t g_littlefs_mount =
{
    .mount_point = FS_MOUNT_NOR,
    .ops = &g_littlefs_ops,
    .ctx = 0,
    .mounted = false,
    .open_count = 0U,
};

fs_mount_t *fs_littlefs_mount_obj(void)
{
    return &g_littlefs_mount;
}
