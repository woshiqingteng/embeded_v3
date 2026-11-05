#ifndef BSP_CORE_H
#define BSP_CORE_H

#include <stdint.h>

int bsp_core_init(void);

void bsp_wdg_init(void);
void bsp_wdg_feed(void);

#endif /* BSP_CORE_H */
