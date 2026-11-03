#ifndef BSP_LOG_H
#define BSP_LOG_H

#include <stdint.h>

typedef void (*bsp_log_sink_t)(const uint8_t *data, uint32_t len);

void bsp_log_init(void);
void bsp_log_attach_uart(void);
void bsp_log_attach_sink(bsp_log_sink_t sink);

void bsp_log_write(const uint8_t *data, uint32_t len);
void bsp_log_poll(void);
uint32_t bsp_log_dropped(void);

#endif /* BSP_LOG_H */
