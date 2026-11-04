/**
 * @file    startup_stm32f429igtx.c
 * @brief   C startup: initialise .data/.bss, then SystemInit() -> C runtime -> main().
 */

#include <stdint.h>

extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern int  main(void);
extern void SystemInit(void);
extern void __libc_init_array(void);

/** @brief Reset entry point (referenced by the vector table as the initial PC). */
void Reset_Handler(void);

void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    while (dst < &_edata)
    {
        *dst++ = *src++;
    }

    for (dst = &_sbss; dst < &_ebss; )
    {
        *dst++ = 0U;
    }

    SystemInit();

    __libc_init_array();

    main();

    for (;;)
    {
    }
}
