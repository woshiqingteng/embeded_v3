#ifndef MW_FS_H
#define MW_FS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FS_MOUNT_SD    "/sd:"
#define FS_MOUNT_FLASH "/flash:"
#define FS_MOUNT_NOR   "/nor:"

#define FS_NAME_MAX 64U

#define FS_O_READ   0x0001U
#define FS_O_WRITE  0x0002U
#define FS_O_CREATE 0x0004U
#define FS_O_APPEND 0x0008U
#define FS_O_TRUNC  0x0010U

#define FS_DIR_ENTRY_FILE 1U
#define FS_DIR_ENTRY_DIR  2U

typedef uint32_t fs_mode_t;

typedef struct fs_mount fs_mount_t;
typedef struct fs_file fs_file_t;
typedef struct fs_dir fs_dir_t;

struct fs_dirent
{
    char     name[FS_NAME_MAX];
    size_t   size;
    uint8_t  type;
};

typedef struct fs_dirent fs_dirent_t;

typedef struct
{
    unsigned long block_size;
    unsigned long total_blocks;
    unsigned long free_blocks;
    unsigned long total_bytes;
    unsigned long free_bytes;
} fs_statvfs_t;

struct fs_file
{
    fs_mount_t *mp;
    void       *priv;
    uint32_t    flags;
};

struct fs_dir
{
    fs_mount_t *mp;
    void       *priv;
};

int fs_init(void);

int fs_mount(fs_mount_t *mp);
int fs_unmount(fs_mount_t *mp);
bool fs_is_mounted(const char *path);

int fs_open(fs_file_t *fp, const char *path, fs_mode_t flags);
int fs_close(fs_file_t *fp);
ssize_t fs_read(fs_file_t *fp, void *buf, size_t size);
ssize_t fs_write(fs_file_t *fp, const void *buf, size_t size);
off_t fs_seek(fs_file_t *fp, off_t off, int whence);
off_t fs_tell(fs_file_t *fp);
int fs_truncate(fs_file_t *fp, off_t length);
int fs_sync(fs_file_t *fp);

int fs_stat(const char *path, fs_dirent_t *entry);
int fs_unlink(const char *path);
int fs_rename(const char *old_path, const char *new_path);
int fs_mkdir(const char *path);

int fs_opendir(fs_dir_t *dp, const char *path);
int fs_readdir(fs_dir_t *dp, fs_dirent_t *entry);
int fs_closedir(fs_dir_t *dp);

int fs_statvfs(const char *path, fs_statvfs_t *stat);

fs_mount_t *fs_fatfs_mount_obj(void);
fs_mount_t *fs_littlefs_mount_obj(void);
fs_mount_t *fs_logfs_mount_obj(void);

#ifdef __cplusplus
}
#endif

#endif /* MW_FS_H */
