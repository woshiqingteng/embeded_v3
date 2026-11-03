/**
 * @file    dma_hw.h
 * @brief   Generic DMA stream hardware attribute (identity + full Init).
 *
 * Shared by board drivers (ADC, USART, ...) that embed a dma_hw_t to describe
 * one DMA stream. dma_hw_setup() enables the controller clock and applies the
 * Init fields; the caller supplies the transfer addresses/length at start time
 * and owns the NVIC (via @c irqn).
 */

#ifndef BSP_DMA_HW_H
#define BSP_DMA_HW_H

#include <stdint.h>

#include "stm32f4xx_hal.h"

/** @brief DMA stream hardware attributes. */
typedef struct
{
    uint32_t            rcc_en;         /*!< RCC AHB1ENR clock-enable bit */
    DMA_Stream_TypeDef *stream;         /*!< DMAx_Streamy */
    IRQn_Type           irqn;           /*!< DMAx_Streamy_IRQn (driver NVIC) */
    uint32_t            channel;        /*!< DMA_CHANNEL_n */
    uint32_t            direction;      /*!< DMA_PERIPH_TO_MEMORY / DMA_MEMORY_TO_PERIPH */
    uint32_t            periph_inc;     /*!< DMA_PINC_* */
    uint32_t            mem_inc;        /*!< DMA_MINC_* */
    uint32_t            periph_align;   /*!< DMA_PDATAALIGN_* */
    uint32_t            mem_align;      /*!< DMA_MDATAALIGN_* */
    uint32_t            mode;           /*!< DMA_NORMAL / DMA_CIRCULAR */
    uint32_t            priority;       /*!< DMA_PRIORITY_* */
    uint32_t            fifo_mode;      /*!< DMA_FIFOMODE_* */
    uint32_t            fifo_threshold; /*!< DMA_FIFO_THRESHOLD_* */
    uint32_t            mem_burst;      /*!< DMA_MBURST_* */
    uint32_t            periph_burst;   /*!< DMA_PBURST_* */
} dma_hw_t;

/**
 * @brief  Enable the DMA controller clock and initialise @p hdma from @p hw.
 * @note   The NVIC is not touched here; the driver enables @c hw->irqn itself.
 */
static inline void dma_hw_setup(DMA_HandleTypeDef *hdma, const dma_hw_t *hw)
{
    SET_BIT(RCC->AHB1ENR, hw->rcc_en);

    hdma->Instance                     = hw->stream;
    hdma->Init.Channel                 = hw->channel;
    hdma->Init.Direction               = hw->direction;
    hdma->Init.PeriphInc               = hw->periph_inc;
    hdma->Init.MemInc                  = hw->mem_inc;
    hdma->Init.PeriphDataAlignment     = hw->periph_align;
    hdma->Init.MemDataAlignment        = hw->mem_align;
    hdma->Init.Mode                    = hw->mode;
    hdma->Init.Priority                = hw->priority;
    hdma->Init.FIFOMode                = hw->fifo_mode;
    hdma->Init.FIFOThreshold           = hw->fifo_threshold;
    hdma->Init.MemBurst                = hw->mem_burst;
    hdma->Init.PeriphBurst             = hw->periph_burst;
    (void)HAL_DMA_Init(hdma);
}

#endif /* BSP_DMA_HW_H */
