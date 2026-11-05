#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "bsp_log.h"
#include "rb.h"

#define BSP_LOG_RING_SIZE 8192U
#define BSP_LOG_CHUNK     256U

static uint8_t           g_log_mem[BSP_LOG_RING_SIZE];
static ringbuf_t         g_log_rb;
static SemaphoreHandle_t g_log_mutex;
static volatile uint32_t g_dropped;
static bsp_log_sink_t    g_console_sink;
static bsp_log_sink_t    g_file_sink;
static volatile uint8_t  g_log_ready;

void bsp_log_init(void)
{
    rb_init(&g_log_rb, g_log_mem, BSP_LOG_RING_SIZE);
    g_log_mutex = 0;
    g_dropped = 0U;
    g_console_sink = 0;
    g_file_sink = 0;
    g_log_ready = 1U;
}

void bsp_log_start(void)
{
    if (g_log_mutex == 0)
    {
        g_log_mutex = xSemaphoreCreateMutex();
    }
}

void bsp_log_attach_console(bsp_log_sink_t sink)
{
    g_console_sink = sink;
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

    if (g_log_mutex == 0)
    {
        /* Scheduler not started yet: boot code is single-threaded. */
        put = rb_put(&g_log_rb, data, len);
        if (put < len)
        {
            g_dropped += (len - put);
        }
        return;
    }

    if (xSemaphoreTake(g_log_mutex, 0U) != pdTRUE)
    {
        g_dropped += len;
        return;
    }

    put = rb_put(&g_log_rb, data, len);
    if (put < len)
    {
        g_dropped += (len - put);
    }

    (void)xSemaphoreGive(g_log_mutex);
}

static void bsp_log_vprintf(const char *source, bsp_log_level_t level,
                            const char *tag, const char *fmt, va_list ap)
{
    static const char level_ch[] = { 'E', 'W', 'I', 'D' };
    char buf[256];
    int n;
    int m;

    if ((g_log_ready == 0U) || (source == 0) || (tag == 0) || (fmt == 0))
    {
        return;
    }

    n = snprintf(buf, sizeof(buf), "[%s/%c] %s: ",
                 source,
                 level_ch[level < BSP_LOG_DEBUG ? (int)level : (int)BSP_LOG_DEBUG],
                 tag);
    if (n < 0)
    {
        return;
    }
    if ((size_t)n >= sizeof(buf))
    {
        n = (int)(sizeof(buf) - 1U);
    }

    m = vsnprintf(buf + n, sizeof(buf) - (size_t)n, fmt, ap);
    if (m < 0)
    {
        return;
    }

    n += m;
    if ((size_t)n > (sizeof(buf) - 3U))
    {
        n = (int)(sizeof(buf) - 3U);
    }
    buf[n++] = '\r';
    buf[n++] = '\n';
    bsp_log_write((const uint8_t *)buf, (uint32_t)n);
}

void bsp_log_printf(bsp_log_level_t level, const char *tag,
                    const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    bsp_log_vprintf("BSP", level, tag, fmt, ap);
    va_end(ap);
}

void bsp_log_poll(void)
{
    uint8_t chunk[BSP_LOG_CHUNK];

    for (;;)
    {
        uint32_t n = rb_get(&g_log_rb, chunk, sizeof(chunk));

        if (n == 0U)
        {
            break;
        }

        if (g_console_sink != 0)
        {
            g_console_sink(chunk, n);
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
