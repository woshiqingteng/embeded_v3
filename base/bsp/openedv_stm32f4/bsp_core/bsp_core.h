#ifndef BSP_CORE_H
#define BSP_CORE_H

#include <stdint.h>

int bsp_core_init(void);

uint32_t bsp_core_uptime_ms(void);
void bsp_core_get_clock_mhz(uint32_t *sysclk, uint32_t *hclk,
                            uint32_t *pclk1, uint32_t *pclk2);

void bsp_wdg_init(void);
void bsp_wdg_feed(void);

#endif /* BSP_CORE_H */
