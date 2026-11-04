#include "stm32f4xx_hal.h"

#include "fault_info.h"

__attribute__((section(".noinit"), used))
volatile app_fault_info_t g_fault_info;

void HardFault_Handler(void)
{
    uint32_t lr;
    uint32_t *frame;

    __asm volatile ("mov %0, lr" : "=r" (lr));

    frame = ((lr & 0x4U) != 0U) ? (uint32_t *)__get_PSP() : (uint32_t *)__get_MSP();

    g_fault_info.magic = APP_FAULT_MAGIC;
    g_fault_info.cfsr  = SCB->CFSR;
    g_fault_info.hfsr  = SCB->HFSR;
    g_fault_info.mmfar = SCB->MMFAR;
    g_fault_info.bfar  = SCB->BFAR;
    g_fault_info.pc    = frame[6];
    g_fault_info.lr    = frame[5];
    g_fault_info.psp   = __get_PSP();
    g_fault_info.msp   = __get_MSP();
    g_fault_info.xpsr  = frame[7];

    __DSB();
    NVIC_SystemReset();
}
