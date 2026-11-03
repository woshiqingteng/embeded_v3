#include <stddef.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "bsp_log.h"
#include "bsp_uart.h"
#include "ringbuf.h"

#define BSP_LOG_RING_SIZE 8192U
#define BSP_LOG_CHUNK     256U

static uint8_t           g_log_mem[BSP_LOG_RING_SIZE];
static ringbuf_t         g_log_rb;
static SemaphoreHandle_t g_log_mutex;
static volatile uint32_t g_dropped;
static volatile uint8_t  g_uart_enabled;
static bsp_log_sink_t    g_file_sink;

void bsp_log_init(void)
{
    ringbuf_init(&g_log_rb, g_log_mem, BSP_LOG_RING_SIZE);
    g_log_mutex = xSemaphoreCreateMutex();
    g_dropped = 0U;
    g_uart_enabled = 0U;
    g_file_sink = 0;
}

void bsp_log_attach_uart(void)
{
    g_uart_enabled = 1U;
}

void bsp_log_attach_sink(bsp_log_sink_t sink)
{
    g_file_sink = sink;
}

void bsp_log_write(const uint8_t *data, uint32_t len)
{
    uint32_t put;

    if ((data == 0) || (len == 0U))
    {
        return;
    }

    if ((g_log_mutex == 0) ||
        (xSemaphoreTake(g_log_mutex, 0U) != pdTRUE))
    {
        g_dropped += len;
        return;
    }

    put = ringbuf_put(&g_log_rb, data, len);
    if (put < len)
    {
        g_dropped += (len - put);
    }

    (void)xSemaphoreGive(g_log_mutex);
}

void bsp_log_poll(void)
{
    uint8_t chunk[BSP_LOG_CHUNK];

    for (;;)
    {
        uint32_t n = ringbuf_get(&g_log_rb, chunk, sizeof(chunk));

        if (n == 0U)
        {
            break;
        }

        if (g_uart_enabled)
        {
            (void)bsp_uart_write(BSP_UART1, chunk, n, 0U);
        }

        if (g_file_sink != 0)
        {
            g_file_sink(chunk, n);
        }
    }
}

uint32_t bsp_log_dropped(void)
{
    return g_dropped;
}
