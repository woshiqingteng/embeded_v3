#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdbool.h>
#include <stdint.h>

#define BSP_ADC_CHANNELS 6U

void bsp_adc_init(void);
bool bsp_adc_get_mv(uint16_t mv[BSP_ADC_CHANNELS]);

#endif /* BSP_ADC_H */
