#ifndef BSP_OLED_SSD1306_H
#define BSP_OLED_SSD1306_H

#include <stdint.h>

void oled_ssd1306_init(void);
void oled_ssd1306_display_on(void);
void oled_ssd1306_display_off(void);
void oled_ssd1306_flush(const uint8_t *gram, uint8_t width, uint8_t pages);

#endif /* BSP_OLED_SSD1306_H */
