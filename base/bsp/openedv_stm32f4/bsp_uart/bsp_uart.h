#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BSP_UART1 = 0, /* PA9/PA10, log + shell */
    BSP_UART2,     /* PA2/PA3, periodic frame */
    BSP_UART_NUM
} bsp_uart_t;

int bsp_uart_init(void);

uint32_t bsp_uart_get_baud(bsp_uart_t id);

uint32_t bsp_uart_write(bsp_uart_t id, const uint8_t *data, uint32_t len,
                        uint32_t timeout_ms);
uint32_t bsp_uart_read(bsp_uart_t id, uint8_t *data, uint32_t len);
bool bsp_uart_tx_busy(bsp_uart_t id);

#endif /* BSP_UART_H */
