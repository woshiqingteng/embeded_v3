#ifndef MW_FATFS_H
#define MW_FATFS_H

#include <stdbool.h>
#include <stdint.h>

#include "ff.h"

int  mw_fatfs_init(void);
int  mw_fatfs_mount(void);
int  mw_fatfs_umount(void);
bool mw_fatfs_is_mounted(void);

int  mw_fatfs_lock(uint32_t timeout_ms);
void mw_fatfs_unlock(void);

#endif /* MW_FATFS_H */
