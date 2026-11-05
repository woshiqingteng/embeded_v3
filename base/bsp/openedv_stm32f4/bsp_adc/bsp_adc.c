#include "stm32f4xx_hal.h"
#include "bsp_adc.h"
#include "bsp_log.h"

#define ADC_SAMPLES_PER_CHANNEL 32U
#define ADC_BLOCK_SAMPLES       (BSP_ADC_CHANNELS * ADC_SAMPLES_PER_CHANNEL)
#define ADC_TOTAL_SAMPLES       (ADC_BLOCK_SAMPLES * 2U)

static ADC_HandleTypeDef  g_hadc;
static DMA_HandleTypeDef  g_hdma_adc;
static uint16_t           g_dma_buf[ADC_TOTAL_SAMPLES];
static volatile uint16_t  g_snapshot[BSP_ADC_CHANNELS];
static volatile uint8_t   g_dirty;

static void adc_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    gpio.Pin  = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 |
                GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio);
}

static void adc_dma_init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();

    g_hdma_adc.Instance                 = DMA2_Stream4;
    g_hdma_adc.Init.Channel             = DMA_CHANNEL_0;
    g_hdma_adc.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    g_hdma_adc.Init.PeriphInc           = DMA_PINC_DISABLE;
    g_hdma_adc.Init.MemInc              = DMA_MINC_ENABLE;
    g_hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    g_hdma_adc.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    g_hdma_adc.Init.Mode                = DMA_CIRCULAR;
    g_hdma_adc.Init.Priority            = DMA_PRIORITY_MEDIUM;
    g_hdma_adc.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    (void)HAL_DMA_Init(&g_hdma_adc);

    __HAL_LINKDMA(&g_hadc, DMA_Handle, g_hdma_adc);

    HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
}

void bsp_adc_init(void)
{
    ADC_ChannelConfTypeDef ch = {0};
    const uint32_t channels[BSP_ADC_CHANNELS] =
    {
        ADC_CHANNEL_10, ADC_CHANNEL_11, ADC_CHANNEL_12,
        ADC_CHANNEL_13, ADC_CHANNEL_14, ADC_CHANNEL_15,
    };

    __HAL_RCC_ADC1_CLK_ENABLE();
    adc_gpio_init();

    g_hadc.Instance                   = ADC1;
    g_hadc.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    g_hadc.Init.Resolution            = ADC_RESOLUTION_12B;
    g_hadc.Init.ScanConvMode          = ENABLE;
    g_hadc.Init.ContinuousConvMode    = ENABLE;
    g_hadc.Init.DiscontinuousConvMode = DISABLE;
    g_hadc.Init.NbrOfDiscConversion   = 0U;
    g_hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    g_hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    g_hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    g_hadc.Init.NbrOfConversion       = BSP_ADC_CHANNELS;
    g_hadc.Init.DMAContinuousRequests = ENABLE;
    g_hadc.Init.EOCSelection          = ADC_EOC_SEQ_CONV;
    (void)HAL_ADC_Init(&g_hadc);

    for (uint8_t i = 0U; i < BSP_ADC_CHANNELS; i++)
    {
        ch.Channel      = channels[i];
        ch.Rank         = (uint32_t)(i + 1U);
        ch.SamplingTime = ADC_SAMPLETIME_480CYCLES;
        ch.Offset       = 0U;
        (void)HAL_ADC_ConfigChannel(&g_hadc, &ch);
    }

    adc_dma_init();
    (void)HAL_ADC_Start_DMA(&g_hadc, (uint32_t *)g_dma_buf, ADC_TOTAL_SAMPLES);
    BSP_LOGI("adc", "ADC1 6 channels ready");
}

static void adc_process_block(uint32_t offset)
{
    for (uint8_t ch = 0U; ch < BSP_ADC_CHANNELS; ch++)
    {
        uint32_t sum = 0U;

        for (uint8_t i = 0U; i < ADC_SAMPLES_PER_CHANNEL; i++)
        {
            sum += g_dma_buf[offset + ((uint32_t)i * BSP_ADC_CHANNELS) + ch];
        }

        g_snapshot[ch] = (uint16_t)((sum * 3300U) /
                                    (ADC_SAMPLES_PER_CHANNEL * 4095U));
    }
    g_dirty = 1U;
}

void DMA2_Stream4_IRQHandler(void)
{
    if (__HAL_DMA_GET_FLAG(&g_hdma_adc, DMA_FLAG_HTIF0_4) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(&g_hdma_adc, DMA_FLAG_HTIF0_4);
        adc_process_block(0U);
    }
    if (__HAL_DMA_GET_FLAG(&g_hdma_adc, DMA_FLAG_TCIF0_4) != RESET)
    {
        __HAL_DMA_CLEAR_FLAG(&g_hdma_adc, DMA_FLAG_TCIF0_4);
        adc_process_block(ADC_BLOCK_SAMPLES);
    }
    if ((__HAL_DMA_GET_FLAG(&g_hdma_adc, DMA_FLAG_TEIF0_4) != RESET) ||
        (__HAL_DMA_GET_FLAG(&g_hdma_adc, DMA_FLAG_DMEIF0_4) != RESET) ||
        (__HAL_DMA_GET_FLAG(&g_hdma_adc, DMA_FLAG_FEIF0_4) != RESET))
    {
        __HAL_DMA_CLEAR_FLAG(&g_hdma_adc,
                             DMA_FLAG_TEIF0_4 | DMA_FLAG_DMEIF0_4 | DMA_FLAG_FEIF0_4);
        __HAL_DMA_DISABLE(&g_hdma_adc);
        g_hdma_adc.Instance->CR &= ~DMA_SxCR_EN;
    }
}

bool bsp_adc_get_mv(uint16_t mv[BSP_ADC_CHANNELS])
{
    uint32_t primask = __get_PRIMASK();
    bool fresh;

    __disable_irq();
    for (uint8_t i = 0U; i < BSP_ADC_CHANNELS; i++)
    {
        mv[i] = g_snapshot[i];
    }
    fresh = (g_dirty != 0U);
    g_dirty = 0U;
    __set_PRIMASK(primask);

    return fresh;
}
