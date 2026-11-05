#ifndef BSP_KEY_H
#define BSP_KEY_H

#include <stdbool.h>
#include <stdint.h>

#include "key_fsm.h"

typedef enum
{
    BSP_KEY0 = 0,
    BSP_KEY1,
    BSP_KEY2,
    BSP_KEY_WKUP,
    BSP_KEY_NUM,
    BSP_KEY_NONE = BSP_KEY_NUM
} bsp_key_id_t;

typedef struct
{
    bsp_key_id_t id;
    key_action_t action;
} bsp_key_event_t;

void bsp_key_init(void);

/**
 * @brief Scan all keys at 10 ms cadence and return pending events.
 * @return event count written to @p events (0..max_events)
 */
uint8_t bsp_key_scan_10ms(bsp_key_event_t *events, uint8_t max_events);

bool bsp_key_is_pressed(bsp_key_id_t key);

#endif /* BSP_KEY_H */
