#include <string.h>

#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "bsp_uart.h"
#include "ringbuf.h"

#define UART_TX_RING_SIZE 2048U
#define UART_RX_RING_SIZE 512U
#define UART_TX_CHUNK     256U

typedef struct
{
    USART_TypeDef      *instance;
    IRQn_Type           irqn;
    GPIO_TypeDef       *gpio_port;
    uint32_t            gpio_clk;
    uint16_t            tx_pin;
    uint16_t            rx_pin;
    uint8_t             af;
    DMA_Stream_TypeDef *dma_stream;
    uint32_t            dma_clk;
    uint32_t            dma_channel;
    IRQn_Type           dma_irqn;
} uart_hw_t;

static const uart_hw_t s_hw[BSP_UART_NUM] =
{
    {
        USART1, USART1_IRQn, GPIOA, RCC_AHB1ENR_GPIOAEN,
        GPIO_PIN_9, GPIO_PIN_10, GPIO_AF7_USART1,
        DMA2_Stream7, RCC_AHB1ENR_DMA2EN, DMA_CHANNEL_4, DMA2_Stream7_IRQn
    },
    {
        USART2, USART2_IRQn, GPIOA, RCC_AHB1ENR_GPIOAEN,
        GPIO_PIN_2, GPIO_PIN_3, GPIO_AF7_USART2,
        DMA1_Stream6, RCC_AHB1ENR_DMA1EN, DMA_CHANNEL_4, DMA1_Stream6_IRQn
    },
};

static UART_HandleTypeDef g_uart[BSP_UART_NUM];
static DMA_HandleTypeDef  g_dma_tx[BSP_UART_NUM];
static uint8_t            g_tx_mem[BSP_UART_NUM][UART_TX_RING_SIZE];
static uint8_t            g_rx_mem[BSP_UART_NUM][UART_RX_RING_SIZE];
static uint8_t            g_tx_chunk[BSP_UART_NUM][UART_TX_CHUNK];
static uint8_t            g_rx_byte[BSP_UART_NUM];
static ringbuf_t          g_tx_rb[BSP_UART_NUM];
static ringbuf_t          g_rx_rb[BSP_UART_NUM];
static SemaphoreHandle_t  g_tx_mutex[BSP_UART_NUM];
static volatile bool      g_tx_busy[BSP_UART_NUM];

static void uart_gpio_init(const uart_hw_t *hw)
{
    GPIO_InitTypeDef gpio = {0};

    SET_BIT(RCC->AHB1ENR, hw->gpio_clk);
    gpio.Pin       = hw->tx_pin | hw->rx_pin;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = hw->af;
    HAL_GPIO_Init(hw->gpio_port, &gpio);
}

static void uart_dma_init(uint8_t id)
{
    const uart_hw_t *hw = &s_hw[id];

    SET_BIT(RCC->AHB1ENR, hw->dma_clk);

    g_dma_tx[id].Instance                 = hw->dma_stream;
    g_dma_tx[id].Init.Channel             = hw->dma_channel;
    g_dma_tx[id].Init.Direction           = DMA_MEMORY_TO_PERIPH;
    g_dma_tx[id].Init.PeriphInc           = DMA_PINC_DISABLE;
    g_dma_tx[id].Init.MemInc              = DMA_MINC_ENABLE;
    g_dma_tx[id].Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    g_dma_tx[id].Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    g_dma_tx[id].Init.Mode                = DMA_NORMAL;
    g_dma_tx[id].Init.Priority            = DMA_PRIORITY_MEDIUM;
    g_dma_tx[id].Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    (void)HAL_DMA_Init(&g_dma_tx[id]);

    __HAL_LINKDMA(&g_uart[id], hdmatx, g_dma_tx[id]);

    HAL_NVIC_SetPriority(hw->dma_irqn, 5U, 0U);
    HAL_NVIC_EnableIRQ(hw->dma_irqn);
}

static void uart_tx_kick(uint8_t id)
{
    uint32_t primask = __get_PRIMASK();
    uint32_t n;

    if (id >= BSP_UART_NUM)
    {
        return;
    }

    __disable_irq();
    if (g_tx_busy[id])
    {
        __set_PRIMASK(primask);
        return;
    }

    n = ringbuf_get(&g_tx_rb[id], g_tx_chunk[id], UART_TX_CHUNK);
    if (n == 0U)
    {
        __set_PRIMASK(primask);
        return;
    }

    g_tx_busy[id] = true;
    __set_PRIMASK(primask);

    if (HAL_UART_Transmit_DMA(&g_uart[id], g_tx_chunk[id], (uint16_t)n) != HAL_OK)
    {
        g_tx_busy[id] = false;
    }
}

void bsp_uart_init(void)
{
    for (uint8_t id = 0U; id < BSP_UART_NUM; id++)
    {
        const uart_hw_t *hw = &s_hw[id];

        if (id == BSP_UART1)
        {
            __HAL_RCC_USART1_CLK_ENABLE();
        }
        else
        {
            __HAL_RCC_USART2_CLK_ENABLE();
        }

        uart_gpio_init(hw);
        ringbuf_init(&g_tx_rb[id], g_tx_mem[id], UART_TX_RING_SIZE);
        ringbuf_init(&g_rx_rb[id], g_rx_mem[id], UART_RX_RING_SIZE);
        g_tx_busy[id] = false;
        g_tx_mutex[id] = xSemaphoreCreateMutex();

        g_uart[id].Instance          = hw->instance;
        g_uart[id].Init.BaudRate     = 115200U;
        g_uart[id].Init.WordLength   = UART_WORDLENGTH_8B;
        g_uart[id].Init.StopBits     = UART_STOPBITS_1;
        g_uart[id].Init.Parity       = UART_PARITY_NONE;
        g_uart[id].Init.Mode         = UART_MODE_TX_RX;
        g_uart[id].Init.HwFlowCtl    = UART_HWCONTROL_NONE;
        g_uart[id].Init.OverSampling = UART_OVERSAMPLING_16;
        (void)HAL_UART_Init(&g_uart[id]);

        uart_dma_init(id);

        HAL_NVIC_SetPriority(hw->irqn, 5U, 0U);
        HAL_NVIC_EnableIRQ(hw->irqn);
    }

    (void)HAL_UART_Receive_IT(&g_uart[BSP_UART1], &g_rx_byte[BSP_UART1], 1U);
}

uint32_t bsp_uart_write(bsp_uart_t id, const uint8_t *data, uint32_t len,
                        uint32_t timeout_ms)
{
    uint32_t written = 0U;
    TickType_t timeout;
    BaseType_t has_sched;

    if ((id >= BSP_UART_NUM) || (data == 0) || (len == 0U))
    {
        return 0U;
    }

    has_sched = (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) ? pdTRUE : pdFALSE;
    timeout = (timeout_ms == UINT32_MAX) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);

    if (g_tx_mutex[id] == 0)
    {
        return 0U;
    }

    if (xSemaphoreTake(g_tx_mutex[id], has_sched ? timeout : 0U) != pdTRUE)
    {
        return 0U;
    }

    while (written < len)
    {
        uint32_t space = ringbuf_free(&g_tx_rb[id]);
        uint32_t n;

        if (space == 0U)
        {
            if (!has_sched || (timeout_ms == 0U))
            {
                break;
            }
            uart_tx_kick((uint8_t)id);
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        n = len - written;
        if (n > space)
        {
            n = space;
        }
        (void)ringbuf_put(&g_tx_rb[id], data + written, n);
        written += n;
        uart_tx_kick((uint8_t)id);
    }

    (void)xSemaphoreGive(g_tx_mutex[id]);
    return written;
}

uint32_t bsp_uart_read(bsp_uart_t id, uint8_t *data, uint32_t len)
{
    if ((id >= BSP_UART_NUM) || (data == 0) || (len == 0U))
    {
        return 0U;
    }
    return ringbuf_get(&g_rx_rb[id], data, len);
}

bool bsp_uart_tx_busy(bsp_uart_t id)
{
    return (id < BSP_UART_NUM) ? g_tx_busy[id] : false;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    for (uint8_t id = 0U; id < BSP_UART_NUM; id++)
    {
        if (huart->Instance == s_hw[id].instance)
        {
            g_tx_busy[id] = false;
            uart_tx_kick(id);
            return;
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        (void)ringbuf_put_byte(&g_rx_rb[BSP_UART1], g_rx_byte[BSP_UART1]);
        (void)HAL_UART_Receive_IT(&g_uart[BSP_UART1], &g_rx_byte[BSP_UART1], 1U);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        (void)HAL_UART_Receive_IT(&g_uart[BSP_UART1], &g_rx_byte[BSP_UART1], 1U);
    }
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart[BSP_UART1]);
}

void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&g_uart[BSP_UART2]);
}

void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_dma_tx[BSP_UART1]);
}

void DMA1_Stream6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_dma_tx[BSP_UART2]);
}

int __io_putchar(int ch)
{
    uint8_t c = (uint8_t)ch;
    (void)bsp_uart_write(BSP_UART1, &c, 1U, 10U);
    return ch;
}
