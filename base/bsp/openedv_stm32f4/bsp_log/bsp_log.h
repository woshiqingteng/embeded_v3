#ifndef BSP_LOG_H
#define BSP_LOG_H

#include <stdint.h>

typedef void (*bsp_log_sink_t)(const uint8_t *data, uint32_t len);

typedef enum
{
    BSP_LOG_ERROR = 0,
    BSP_LOG_WARN,
    BSP_LOG_INFO,
    BSP_LOG_DEBUG,
} bsp_log_level_t;

void bsp_log_init(void);
void bsp_log_start(void);
void bsp_log_attach_console(bsp_log_sink_t sink);
void bsp_log_attach_sink(bsp_log_sink_t sink);

void bsp_log_write(const uint8_t *data, uint32_t len);
void bsp_log_printf(bsp_log_level_t level, const char *tag,
                    const char *fmt, ...);
void bsp_log_poll(void);
uint32_t bsp_log_dropped(void);

#define BSP_LOGE(tag, fmt, ...) bsp_log_printf(BSP_LOG_ERROR, tag, fmt, ##__VA_ARGS__)
#define BSP_LOGW(tag, fmt, ...) bsp_log_printf(BSP_LOG_WARN,  tag, fmt, ##__VA_ARGS__)
#define BSP_LOGI(tag, fmt, ...) bsp_log_printf(BSP_LOG_INFO,  tag, fmt, ##__VA_ARGS__)
#define BSP_LOGD(tag, fmt, ...) bsp_log_printf(BSP_LOG_DEBUG, tag, fmt, ##__VA_ARGS__)

#endif /* BSP_LOG_H */
