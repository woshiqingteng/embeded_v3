#ifndef APP_FAULT_INFO_H
#define APP_FAULT_INFO_H

#include <stdint.h>

#define APP_FAULT_MAGIC 0x48465254U

typedef struct
{
    uint32_t magic;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t pc;
    uint32_t lr;
    uint32_t psp;
    uint32_t msp;
    uint32_t xpsr;
} app_fault_info_t;

extern volatile app_fault_info_t g_fault_info;

#endif /* APP_FAULT_INFO_H */
