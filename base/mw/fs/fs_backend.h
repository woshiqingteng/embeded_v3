#ifndef MW_FS_BACKEND_H
#define MW_FS_BACKEND_H

#include "fs.h"

struct fs_ops
{
    int     (*mount)(fs_mount_t *mp);
    int     (*unmount)(fs_mount_t *mp);
    int     (*open)(fs_file_t *fp, const char *path, fs_mode_t flags);
    int     (*close)(fs_file_t *fp);
    ssize_t (*read)(fs_file_t *fp, void *buf, size_t size);
    ssize_t (*write)(fs_file_t *fp, const void *buf, size_t size);
    off_t   (*seek)(fs_file_t *fp, off_t off, int whence);
    off_t   (*tell)(fs_file_t *fp);
    int     (*truncate)(fs_file_t *fp, off_t length);
    int     (*sync)(fs_file_t *fp);
    int     (*stat)(fs_mount_t *mp, const char *path, fs_dirent_t *entry);
    int     (*unlink)(fs_mount_t *mp, const char *path);
    int     (*rename)(fs_mount_t *mp, const char *old_path, const char *new_path);
    int     (*mkdir)(fs_mount_t *mp, const char *path);
    int     (*opendir)(fs_dir_t *dp, const char *path);
    int     (*readdir)(fs_dir_t *dp, fs_dirent_t *entry);
    int     (*closedir)(fs_dir_t *dp);
    int     (*statvfs)(fs_mount_t *mp, fs_statvfs_t *stat);
};

typedef enum
{
    FS_STATE_UNMOUNTED = 0,
    FS_STATE_MOUNTING,
    FS_STATE_MOUNTED,
    FS_STATE_UNMOUNTING
} fs_mount_state_t;

struct fs_mount
{
    const char          *mount_point;
    const struct fs_ops *ops;
    void                *ctx;
    fs_mount_state_t     state;
    bool                 mounted;
    uint32_t             open_count;
};

#endif /* MW_FS_BACKEND_H */
