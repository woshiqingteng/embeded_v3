#ifndef BSP_KEY_H
#define BSP_KEY_H

#include <stdbool.h>

typedef enum
{
    BSP_KEY0 = 0,
    BSP_KEY1,
    BSP_KEY2,
    BSP_KEY_WKUP,
    BSP_KEY_NUM,
    BSP_KEY_NONE = BSP_KEY_NUM
} bsp_key_id_t;

void bsp_key_init(void);
bsp_key_id_t bsp_key_scan_10ms(void);
bool bsp_key_is_pressed(bsp_key_id_t key);

#endif /* BSP_KEY_H */
