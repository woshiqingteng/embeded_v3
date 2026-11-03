#ifndef MW_LITTLEFS_H
#define MW_LITTLEFS_H

#include <stdbool.h>
#include <stdint.h>

#include "lfs.h"

int  mw_littlefs_init(void);
int  mw_littlefs_mount(void);
int  mw_littlefs_umount(void);
int  mw_littlefs_format(void);
bool mw_littlefs_is_mounted(void);

lfs_t *mw_littlefs_handle(void);
int  mw_littlefs_lock(uint32_t timeout_ms);
void mw_littlefs_unlock(void);

#endif /* MW_LITTLEFS_H */
