#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint16_t adc_mv[6];
    uint16_t pwm_permille;
    uint32_t last_key;
    bool     usb_connected;
    uint32_t log_dropped;
} app_state_t;

void app_state_init(void);
void app_state_get(app_state_t *out);
void app_state_set_adc(const uint16_t mv[6]);
void app_state_set_pwm(uint16_t permille);
void app_state_set_key(uint32_t key);
void app_state_set_usb(bool connected);

#endif /* APP_STATE_H */
