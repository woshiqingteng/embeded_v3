#include <string.h>

#include "stm32f4xx_hal.h"

#include "fault_info.h"

__attribute__((section(".noinit"), used))
volatile app_fault_info_t g_fault_info;

static void copy_str(volatile char *dst, size_t size, const char *src)
{
    size_t i = 0U;

    if ((dst == 0) || (size == 0U))
    {
        return;
    }
    if (src != 0)
    {
        while ((i + 1U) < size && (src[i] != '\0'))
        {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

static uint32_t fault_type_from_scb(void)
{
    uint32_t cfsr = SCB->CFSR;

    if ((cfsr & 0x000000FFU) != 0U)
    {
        return APP_FAULT_TYPE_MEM;
    }
    if ((cfsr & 0x0000FF00U) != 0U)
    {
        return APP_FAULT_TYPE_BUS;
    }
    if ((cfsr & 0xFFFF0000U) != 0U)
    {
        return APP_FAULT_TYPE_USAGE;
    }
    return APP_FAULT_TYPE_HARD;
}

static void fault_capture(uint32_t *frame, uint32_t exc_return, uint32_t type)
{
    g_fault_info.magic = APP_FAULT_MAGIC;
    g_fault_info.type = type;
    g_fault_info.cfsr = SCB->CFSR;
    g_fault_info.hfsr = SCB->HFSR;
    g_fault_info.mmfar = SCB->MMFAR;
    g_fault_info.bfar = SCB->BFAR;
    g_fault_info.psp = __get_PSP();
    g_fault_info.msp = __get_MSP();
    g_fault_info.exc_return = exc_return;
    g_fault_info.tick = HAL_GetTick();

    if (frame != 0)
    {
        g_fault_info.r0 = frame[0];
        g_fault_info.r1 = frame[1];
        g_fault_info.r2 = frame[2];
        g_fault_info.r3 = frame[3];
        g_fault_info.r12 = frame[4];
        g_fault_info.lr = frame[5];
        g_fault_info.pc = frame[6];
        g_fault_info.xpsr = frame[7];
    }

    __DSB();
    NVIC_SystemReset();
}

void fault_init(void)
{
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk |
                  SCB_SHCSR_BUSFAULTENA_Msk |
                  SCB_SHCSR_USGFAULTENA_Msk;
    __DSB();
    __ISB();
}

void fault_record_reset(uint32_t type)
{
    g_fault_info.magic = APP_FAULT_MAGIC;
    g_fault_info.type = type;
    g_fault_info.cfsr = SCB->CFSR;
    g_fault_info.hfsr = SCB->HFSR;
    g_fault_info.psp = __get_PSP();
    g_fault_info.msp = __get_MSP();
    g_fault_info.tick = HAL_GetTick();
    __DSB();
    NVIC_SystemReset();
}

void fault_assert(const char *file, uint32_t line)
{
    copy_str(g_fault_info.file, sizeof(g_fault_info.file), file);
    g_fault_info.line = line;
    fault_record_reset(APP_FAULT_TYPE_ASSERT);
}

void fault_stack_overflow(const char *task)
{
    copy_str(g_fault_info.task, sizeof(g_fault_info.task), task);
    fault_record_reset(APP_FAULT_TYPE_STACK);
}

static void fault_handler_common(uint32_t type)
{
    uint32_t lr;
    uint32_t *frame;

    __asm volatile ("mov %0, lr" : "=r" (lr));
    frame = ((lr & 0x4U) != 0U) ? (uint32_t *)__get_PSP() : (uint32_t *)__get_MSP();
    fault_capture(frame, lr, type);
}

void HardFault_Handler(void)
{
    fault_handler_common(APP_FAULT_TYPE_HARD);
}

void MemManage_Handler(void)
{
    fault_handler_common(APP_FAULT_TYPE_MEM);
}

void BusFault_Handler(void)
{
    fault_handler_common(APP_FAULT_TYPE_BUS);
}

void UsageFault_Handler(void)
{
    fault_handler_common(APP_FAULT_TYPE_USAGE);
}

void NMI_Handler(void)
{
    fault_handler_common(APP_FAULT_TYPE_NMI);
}

/* Keep a helper for the generic path when the SCB flags need classification. */
void fault_classify_and_reset(void)
{
    fault_record_reset(fault_type_from_scb());
}
