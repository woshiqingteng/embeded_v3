/**
 * @file    oled.h
 * @brief   SSD1306 128x64 OLED driver over the 8080 8-bit parallel bus.
 */

#ifndef BSP_OLED_H
#define BSP_OLED_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

/** @brief  Selectable ASCII font heights. */
typedef enum
{
    OLED_FONT_12X6  = 12,
    OLED_FONT_6X8   = 8,
    OLED_FONT_8X16  = 16,
    OLED_FONT_24X12 = 24
} oled_font_t;

/** @brief  Initialise the SSD1306 and clear the screen. */
void oled_init(void);

void oled_clear(void);
void oled_display_on(void);
void oled_display_off(void);

/** @brief  Flush the internal frame buffer to the panel. */
void oled_refresh(void);

void oled_show_string(uint8_t x, uint8_t y, const char *str, oled_font_t size);

/** @brief  Draw a right-aligned number of len digits at (x,y). */
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, oled_font_t size);

#endif /* BSP_OLED_H */
