#ifndef LV_CONF_H
#define LV_CONF_H

/* LVGL is vendored for future use.  The comprehensive firmware does not link
 * it by default (ENABLE_LVGL=OFF).  Keep a minimal valid configuration here. */
#define LV_COLOR_DEPTH 1
#define LV_USE_OS      LV_OS_FREERTOS

#endif /* LV_CONF_H */
