#ifndef COMMON_KEY_FSM_H
#define COMMON_KEY_FSM_H

#include <stdint.h>

#define KEY_LEVEL_PRESSED   0U
#define KEY_LEVEL_RELEASED  1U

typedef enum
{
    KEY_ST_IDLE = 0,
    KEY_ST_PRESS_DEBOUNCE,
    KEY_ST_PRESSED,
    KEY_ST_RELEASE_DEBOUNCE
} key_state_t;

typedef enum
{
    KEY_ACT_NONE = 0,
    KEY_ACT_PRESS,
    KEY_ACT_RELEASE
} key_action_t;

typedef struct
{
    key_state_t state;
    uint8_t cnt;
} key_fsm_t;

static inline void key_fsm_init(key_fsm_t *k)
{
    k->state = KEY_ST_IDLE;
    k->cnt = 0U;
}

static inline key_action_t key_fsm_scan(key_fsm_t *k, uint8_t level,
                                        uint8_t debounce_cnt)
{
    key_state_t next_state = k->state;
    key_action_t action = KEY_ACT_NONE;

    switch (k->state)
    {
        case KEY_ST_IDLE:
            if (level == KEY_LEVEL_PRESSED)
            {
                next_state = KEY_ST_PRESS_DEBOUNCE;
            }
            break;

        case KEY_ST_PRESS_DEBOUNCE:
            if (level == KEY_LEVEL_RELEASED)
            {
                next_state = KEY_ST_IDLE;
            }
            else if (++k->cnt >= debounce_cnt)
            {
                next_state = KEY_ST_PRESSED;
                action = KEY_ACT_PRESS;
            }
            break;

        case KEY_ST_PRESSED:
            if (level == KEY_LEVEL_RELEASED)
            {
                next_state = KEY_ST_RELEASE_DEBOUNCE;
            }
            break;

        case KEY_ST_RELEASE_DEBOUNCE:
            if (level == KEY_LEVEL_PRESSED)
            {
                next_state = KEY_ST_PRESSED;
            }
            else if (++k->cnt >= debounce_cnt)
            {
                next_state = KEY_ST_IDLE;
                action = KEY_ACT_RELEASE;
            }
            break;

        default:
            next_state = KEY_ST_IDLE;
            break;
    }

    if (next_state != k->state)
    {
        k->state = next_state;
        k->cnt = 0U;
    }

    return action;
}

#endif /* COMMON_KEY_FSM_H */
