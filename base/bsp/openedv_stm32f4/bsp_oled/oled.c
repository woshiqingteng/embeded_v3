/**
 * @file    oled.c
 * @brief   SSD1306 128x64 OLED panel: graphics and frame buffer.
 *
 * Bus and controller independent: rendering works on a local frame buffer and
 * is handed to the private SSD1306 controller layer on refresh.
 */

#include <stdbool.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "oled.h"
#include "oledfont.h"
#include "bsp_log.h"

int  oled_port_init(void);
void oled_port_reset(void);
int  oled_port_write_cmd(const uint8_t *data, uint16_t len);
int  oled_port_write_data(const uint8_t *data, uint16_t len);

static void oled_controller_init(void);
static void oled_controller_display_on(void);
static void oled_controller_display_off(void);
static void oled_controller_flush(const uint8_t *gram, uint8_t width, uint8_t pages);

/* Panel geometry. */
#define OLED_WIDTH_PX     128U
#define OLED_HEIGHT_PX    64U
#define OLED_PAGE_BIT_COUNT 8U
#define OLED_PAGE_COUNT     (OLED_HEIGHT_PX / OLED_PAGE_BIT_COUNT)

/* Character metrics. */
#define OLED_ASCII_FIRST        0x20U
#define OLED_ASCII_LAST         0x7EU
#define OLED_DECIMAL_BASE       10U

static uint8_t g_oled_gram[OLED_WIDTH_PX][OLED_PAGE_COUNT];
static SemaphoreHandle_t g_oled_mutex;

static void oled_draw_point(uint8_t x, uint8_t y, bool dot)
{
    uint8_t page;
    uint8_t bit;

    if ((x >= OLED_WIDTH_PX) || (y >= OLED_HEIGHT_PX))
    {
        return;
    }

    page = (uint8_t)(y / OLED_PAGE_BIT_COUNT);
    bit  = (uint8_t)(1U << (y % OLED_PAGE_BIT_COUNT));

    if (dot)
    {
        g_oled_gram[x][page] |= bit;
    }
    else
    {
        g_oled_gram[x][page] &= (uint8_t)(~bit);
    }
}

static uint8_t oled_char_width(oled_font_t size)
{
    switch (size)
    {
        case OLED_FONT_6X8:
        case OLED_FONT_12X6:
            return 6U;

        case OLED_FONT_24X12:
            return 12U;

        default:
            return 8U;
    }
}

/* Font bytes are MSB-first: row 0 is bit 7. */
static uint8_t glyph_bit(uint8_t byte, uint8_t row)
{
    return (uint8_t)((byte >> (7U - row)) & 1U);
}

static void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, oled_font_t size)
{
    const uint8_t *pfont;
    uint8_t idx;
    uint8_t width;
    uint8_t height;
    uint8_t bytes_per_col;
    uint8_t col;
    uint8_t b;
    uint8_t row;
    uint8_t font_byte;

    if ((chr < OLED_ASCII_FIRST) || (chr > OLED_ASCII_LAST))
    {
        return;
    }

    idx = (uint8_t)(chr - OLED_ASCII_FIRST);

    if (size == OLED_FONT_6X8)
    {
        pfont = (const uint8_t *)oled_font_6x8[idx];
        width = 6U;
        height = 8U;
        bytes_per_col = 1U;
    }
    else
    {
        height = (uint8_t)size;
        width = (uint8_t)((uint8_t)size / 2U);
        bytes_per_col = (uint8_t)((height + 7U) / 8U);

        switch (size)
        {
            case OLED_FONT_12X6:  pfont = (const uint8_t *)oled_asc2_1206[idx]; break;
            case OLED_FONT_8X16:  pfont = (const uint8_t *)oled_asc2_1608[idx]; break;
            case OLED_FONT_24X12: pfont = (const uint8_t *)oled_asc2_2412[idx]; break;
            default: return;
        }
    }

    for (col = 0U; col < width; col++)
    {
        for (b = 0U; b < bytes_per_col; b++)
        {
            font_byte = pfont[(uint8_t)(bytes_per_col * col) + b];

            for (row = 0U; row < 8U; row++)
            {
                uint8_t py = (uint8_t)((b * 8U) + row);

                if (py >= height)
                {
                    break;
                }

                oled_draw_point((uint8_t)(x + col), (uint8_t)(y + py),
                                glyph_bit(font_byte, row));
            }
        }
    }
}

void oled_refresh(void)
{
    oled_controller_flush((const uint8_t *)g_oled_gram, OLED_WIDTH_PX, OLED_PAGE_COUNT);
}

void oled_display_on(void)
{
    oled_controller_display_on();
}

void oled_display_off(void)
{
    oled_controller_display_off();
}

void oled_clear(void)
{
    uint8_t page;
    uint8_t col;

    for (page = 0; page < OLED_PAGE_COUNT; page++)
    {
        for (col = 0; col < OLED_WIDTH_PX; col++)
        {
            g_oled_gram[col][page] = 0x00U;
        }
    }

    oled_refresh();
}

void oled_show_string(uint8_t x, uint8_t y, const char *str, oled_font_t size)
{
    uint8_t width = oled_char_width(size);

    while ((*str >= (char)OLED_ASCII_FIRST) && (*str <= (char)OLED_ASCII_LAST))
    {
        if (x > (uint8_t)(OLED_WIDTH_PX - width))
        {
            x = 0U;
            y = (uint8_t)(y + size);
        }

        if (y > (uint8_t)(OLED_HEIGHT_PX - size))
        {
            x = 0U;
            y = 0U;
            oled_clear();
        }

        oled_show_char(x, y, (uint8_t)*str, size);
        x = (uint8_t)(x + width);
        str++;
    }
}

void oled_lock(void)
{
    if (g_oled_mutex != 0)
    {
        (void)xSemaphoreTakeRecursive(g_oled_mutex, portMAX_DELAY);
    }
}

void oled_unlock(void)
{
    if (g_oled_mutex != 0)
    {
        (void)xSemaphoreGiveRecursive(g_oled_mutex);
    }
}

void oled_init(void)
{
    if (g_oled_mutex == 0)
    {
        g_oled_mutex = xSemaphoreCreateRecursiveMutex();
    }

    oled_controller_init();
    oled_clear();
    BSP_LOGI("oled", "SSD1306 128x64 ready");
}

static void oled_cmd(const uint8_t *data, uint16_t len)
{
    (void)oled_port_write_cmd(data, len);
}

static void oled_cmd1(uint8_t cmd)
{
    (void)oled_port_write_cmd(&cmd, 1U);
}

static void oled_controller_init(void)
{
    static const uint8_t init_seq[] = {
        0xAE,
        0xD5, 0x50,
        0xA8, 0x3F,
        0xD3, 0x00,
        0x40,
        0x8D, 0x14,
        0x20, 0x02,
        0xA1,
        0xC8,
        0xDA, 0x12,
        0x81, 0xEF,
        0xD9, 0xF1,
        0xDB, 0x30,
        0xA4,
        0xA6,
        0xAF,
    };

    (void)oled_port_init();
    oled_port_reset();
    oled_cmd(init_seq, (uint16_t)sizeof(init_seq));
}

static void oled_controller_display_on(void)
{
    oled_cmd1(0x8D);
    oled_cmd1(0x14);
    oled_cmd1(0xAF);
}

static void oled_controller_display_off(void)
{
    oled_cmd1(0x8D);
    oled_cmd1(0x10);
    oled_cmd1(0xAE);
}

static void oled_controller_flush(const uint8_t *gram, uint8_t width, uint8_t pages)
{
    uint8_t line[128];

    for (uint8_t page = 0U; page < pages; page++)
    {
        oled_cmd1((uint8_t)(0xB0U + page));
        oled_cmd1(0x00);
        oled_cmd1(0x10);

        for (uint8_t col = 0U; col < width; col++)
        {
            line[col] = gram[(uint32_t)col * pages + page];
        }

        (void)oled_port_write_data(line, width);
    }
}
