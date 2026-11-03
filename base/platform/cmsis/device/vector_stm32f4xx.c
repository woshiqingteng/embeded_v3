/**
 * @file    vector_stm32f429igtx.c
 * @brief   STM32F429IGTx interrupt vector table.
 *
 * Symbols resolve to the weak aliases in stm32f4xx_it.c and can be overridden by strong
 * definitions in the bsp modules.
 */

#include <stdint.h>

extern uint32_t _estack;
extern void Reset_Handler(void);

extern void NMI_Handler(void);
extern void HardFault_Handler(void);
extern void MemManage_Handler(void);
extern void BusFault_Handler(void);
extern void UsageFault_Handler(void);
extern void SVC_Handler(void);
extern void DebugMon_Handler(void);
extern void PendSV_Handler(void);
extern void SysTick_Handler(void);

extern void WWDG_IRQHandler(void);
extern void PVD_IRQHandler(void);
extern void TAMP_STAMP_IRQHandler(void);
extern void RTC_WKUP_IRQHandler(void);
extern void FLASH_IRQHandler(void);
extern void RCC_IRQHandler(void);
extern void EXTI0_IRQHandler(void);
extern void EXTI1_IRQHandler(void);
extern void EXTI2_IRQHandler(void);
extern void EXTI3_IRQHandler(void);
extern void EXTI4_IRQHandler(void);
extern void DMA1_Stream0_IRQHandler(void);
extern void DMA1_Stream1_IRQHandler(void);
extern void DMA1_Stream2_IRQHandler(void);
extern void DMA1_Stream3_IRQHandler(void);
extern void DMA1_Stream4_IRQHandler(void);
extern void DMA1_Stream5_IRQHandler(void);
extern void DMA1_Stream6_IRQHandler(void);
extern void ADC_IRQHandler(void);
extern void CAN1_TX_IRQHandler(void);
extern void CAN1_RX0_IRQHandler(void);
extern void CAN1_RX1_IRQHandler(void);
extern void CAN1_SCE_IRQHandler(void);
extern void EXTI9_5_IRQHandler(void);
extern void TIM1_BRK_TIM9_IRQHandler(void);
extern void TIM1_UP_TIM10_IRQHandler(void);
extern void TIM1_TRG_COM_TIM11_IRQHandler(void);
extern void TIM1_CC_IRQHandler(void);
extern void TIM2_IRQHandler(void);
extern void TIM3_IRQHandler(void);
extern void TIM4_IRQHandler(void);
extern void I2C1_EV_IRQHandler(void);
extern void I2C1_ER_IRQHandler(void);
extern void I2C2_EV_IRQHandler(void);
extern void I2C2_ER_IRQHandler(void);
extern void SPI1_IRQHandler(void);
extern void SPI2_IRQHandler(void);
extern void USART1_IRQHandler(void);
extern void USART2_IRQHandler(void);
extern void USART3_IRQHandler(void);
extern void EXTI15_10_IRQHandler(void);
extern void RTC_Alarm_IRQHandler(void);
extern void OTG_FS_WKUP_IRQHandler(void);
extern void TIM8_BRK_TIM12_IRQHandler(void);
extern void TIM8_UP_TIM13_IRQHandler(void);
extern void TIM8_TRG_COM_TIM14_IRQHandler(void);
extern void TIM8_CC_IRQHandler(void);
extern void DMA1_Stream7_IRQHandler(void);
extern void FMC_IRQHandler(void);
extern void SDIO_IRQHandler(void);
extern void TIM5_IRQHandler(void);
extern void SPI3_IRQHandler(void);
extern void UART4_IRQHandler(void);
extern void UART5_IRQHandler(void);
extern void TIM6_DAC_IRQHandler(void);
extern void TIM7_IRQHandler(void);
extern void DMA2_Stream0_IRQHandler(void);
extern void DMA2_Stream1_IRQHandler(void);
extern void DMA2_Stream2_IRQHandler(void);
extern void DMA2_Stream3_IRQHandler(void);
extern void DMA2_Stream4_IRQHandler(void);
extern void ETH_IRQHandler(void);
extern void ETH_WKUP_IRQHandler(void);
extern void CAN2_TX_IRQHandler(void);
extern void CAN2_RX0_IRQHandler(void);
extern void CAN2_RX1_IRQHandler(void);
extern void CAN2_SCE_IRQHandler(void);
extern void OTG_FS_IRQHandler(void);
extern void DMA2_Stream5_IRQHandler(void);
extern void DMA2_Stream6_IRQHandler(void);
extern void DMA2_Stream7_IRQHandler(void);
extern void USART6_IRQHandler(void);
extern void I2C3_EV_IRQHandler(void);
extern void I2C3_ER_IRQHandler(void);
extern void OTG_HS_EP1_OUT_IRQHandler(void);
extern void OTG_HS_EP1_IN_IRQHandler(void);
extern void OTG_HS_WKUP_IRQHandler(void);
extern void OTG_HS_IRQHandler(void);
extern void DCMI_IRQHandler(void);
extern void HASH_RNG_IRQHandler(void);
extern void FPU_IRQHandler(void);
extern void UART7_IRQHandler(void);
extern void UART8_IRQHandler(void);
extern void SPI4_IRQHandler(void);
extern void SPI5_IRQHandler(void);
extern void SPI6_IRQHandler(void);
extern void SAI1_IRQHandler(void);
extern void LTDC_IRQHandler(void);
extern void LTDC_ER_IRQHandler(void);
extern void DMA2D_IRQHandler(void);

typedef void (*isr_handler_t)(void);

__attribute__((section(".isr_vector"), used))
const isr_handler_t g_isr_vector[] =
{
    (isr_handler_t)&_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMP_STAMP_IRQHandler,
    RTC_WKUP_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Stream0_IRQHandler,
    DMA1_Stream1_IRQHandler,
    DMA1_Stream2_IRQHandler,
    DMA1_Stream3_IRQHandler,
    DMA1_Stream4_IRQHandler,
    DMA1_Stream5_IRQHandler,
    DMA1_Stream6_IRQHandler,
    ADC_IRQHandler,
    CAN1_TX_IRQHandler,
    CAN1_RX0_IRQHandler,
    CAN1_RX1_IRQHandler,
    CAN1_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_TIM9_IRQHandler,
    TIM1_UP_TIM10_IRQHandler,
    TIM1_TRG_COM_TIM11_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler,
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,
    RTC_Alarm_IRQHandler,
    OTG_FS_WKUP_IRQHandler,
    TIM8_BRK_TIM12_IRQHandler,
    TIM8_UP_TIM13_IRQHandler,
    TIM8_TRG_COM_TIM14_IRQHandler,
    TIM8_CC_IRQHandler,
    DMA1_Stream7_IRQHandler,
    FMC_IRQHandler,
    SDIO_IRQHandler,
    TIM5_IRQHandler,
    SPI3_IRQHandler,
    UART4_IRQHandler,
    UART5_IRQHandler,
    TIM6_DAC_IRQHandler,
    TIM7_IRQHandler,
    DMA2_Stream0_IRQHandler,
    DMA2_Stream1_IRQHandler,
    DMA2_Stream2_IRQHandler,
    DMA2_Stream3_IRQHandler,
    DMA2_Stream4_IRQHandler,
    ETH_IRQHandler,
    ETH_WKUP_IRQHandler,
    CAN2_TX_IRQHandler,
    CAN2_RX0_IRQHandler,
    CAN2_RX1_IRQHandler,
    CAN2_SCE_IRQHandler,
    OTG_FS_IRQHandler,
    DMA2_Stream5_IRQHandler,
    DMA2_Stream6_IRQHandler,
    DMA2_Stream7_IRQHandler,
    USART6_IRQHandler,
    I2C3_EV_IRQHandler,
    I2C3_ER_IRQHandler,
    OTG_HS_EP1_OUT_IRQHandler,
    OTG_HS_EP1_IN_IRQHandler,
    OTG_HS_WKUP_IRQHandler,
    OTG_HS_IRQHandler,
    DCMI_IRQHandler,
    0,
    HASH_RNG_IRQHandler,
    FPU_IRQHandler,
    UART7_IRQHandler,
    UART8_IRQHandler,
    SPI4_IRQHandler,
    SPI5_IRQHandler,
    SPI6_IRQHandler,
    SAI1_IRQHandler,
    LTDC_IRQHandler,
    LTDC_ER_IRQHandler,
    DMA2D_IRQHandler,
};
