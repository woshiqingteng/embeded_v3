#ifndef APP_FAULT_INFO_H
#define APP_FAULT_INFO_H

#include <stdint.h>

#define APP_FAULT_MAGIC 0x48465254U

typedef enum
{
    APP_FAULT_TYPE_HARD = 1,
    APP_FAULT_TYPE_MEM,
    APP_FAULT_TYPE_BUS,
    APP_FAULT_TYPE_USAGE,
    APP_FAULT_TYPE_NMI,
    APP_FAULT_TYPE_STACK,
    APP_FAULT_TYPE_MALLOC,
    APP_FAULT_TYPE_ASSERT
} app_fault_type_t;

typedef struct
{
    uint32_t magic;
    uint32_t type;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t pc;
    uint32_t lr;
    uint32_t psp;
    uint32_t msp;
    uint32_t xpsr;
    uint32_t exc_return;
    uint32_t tick;
    char     task[16];
    char     file[32];
    uint32_t line;
} app_fault_info_t;

extern volatile app_fault_info_t g_fault_info;

void fault_init(void);
void fault_record_reset(uint32_t type);
void fault_assert(const char *file, uint32_t line);
void fault_stack_overflow(const char *task);

#endif /* APP_FAULT_INFO_H */
