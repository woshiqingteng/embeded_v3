#include <string.h>

#include "app_state.h"
#include "app_ipc.h"

static app_state_t g_state;

void app_state_init(void)
{
    memset(&g_state, 0, sizeof(g_state));
    g_state.pwm_permille = 500U;
}

void app_state_get(app_state_t *out)
{
    if (out == 0)
    {
        return;
    }
    app_state_lock();
    *out = g_state;
    app_state_unlock();
}

void app_state_set_adc(const uint16_t mv[6])
{
    if (mv == 0)
    {
        return;
    }
    app_state_lock();
    for (uint8_t i = 0U; i < 6U; i++)
    {
        g_state.adc_mv[i] = mv[i];
    }
    app_state_unlock();
}

void app_state_set_pwm(uint16_t permille)
{
    app_state_lock();
    g_state.pwm_permille = permille;
    app_state_unlock();
}

void app_state_set_key(uint32_t key)
{
    app_state_lock();
    g_state.last_key = key;
    app_state_unlock();
}

void app_state_set_usb(bool connected)
{
    app_state_lock();
    g_state.usb_connected = connected;
    app_state_unlock();
}
