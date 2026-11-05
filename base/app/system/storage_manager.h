#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include <stdint.h>

typedef enum
{
    STORAGE_STATE_UNINIT = 0,
    STORAGE_STATE_IDLE,
    STORAGE_STATE_FATFS,
    STORAGE_STATE_MSC,
    STORAGE_STATE_MSC_READONLY,
    STORAGE_STATE_ERROR
} storage_state_t;

int  storage_init(void);
int  storage_fatfs_mount(void);
int  storage_fatfs_umount(void);
int  storage_msc_begin(void);
void storage_msc_end(void);
void storage_usb_event(uint8_t event);

storage_state_t storage_state(void);
const char *storage_state_name(void);

#endif /* STORAGE_MANAGER_H */
