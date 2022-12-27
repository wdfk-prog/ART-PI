/**
 * @file get_irq.c
 * @brief 
 * @author HLY (1425075683@qq.com)
 * @version 1.0
 * @date 2022-12-27
 * @copyright Copyright (c) 2022
 * @attention CMSIS中提供了CMSIS access NVIC functions,方便不同芯片查询
 * @par 修改日志:
 * Date       Version Author  Description
 * 2022-12-27 1.0     HLY     first version
 */
/* Includes ------------------------------------------------------------------*/
#include "get_irq.h"
/* Private includes ----------------------------------------------------------*/
#include <rtthread.h>
#include "board.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/******  STM32 specific Interrupt Numbers **********************************************************************/
static const char * const nvic_name[] = {
  [WWDG_IRQn]                   = "WWDG_IRQn",              /*!< Window WatchDog Interrupt ( wwdg1_it, wwdg2_it)                   */
  [PVD_AVD_IRQn]                = "PVD_AVD_IRQn",           /*!< PVD/AVD through EXTI Line detection Interrupt                     */
  [TAMP_STAMP_IRQn]             = "TAMP_STAMP_IRQn",        /*!< Tamper and TimeStamp interrupts through the EXTI line             */
  [RTC_WKUP_IRQn]               = "RTC_WKUP_IRQn",          /*!< RTC Wakeup interrupt through the EXTI line                        */
  [FLASH_IRQn]                  = "FLASH_IRQn",             /*!< FLASH global Interrupt                                            */
  [RCC_IRQn]                    = "RCC_IRQn",               /*!< RCC global Interrupt                                              */
  [EXTI0_IRQn]                  = "EXTI0_IRQn",             /*!< EXTI Line0 Interrupt                                              */
  [EXTI1_IRQn]                  = "EXTI1_IRQn",             /*!< EXTI Line1 Interrupt                                              */
  [EXTI2_IRQn]                  = "EXTI2_IRQn",             /*!< EXTI Line2 Interrupt                                              */
  [EXTI3_IRQn]                  = "EXTI3_IRQn",             /*!< EXTI Line3 Interrupt                                              */
  [EXTI4_IRQn]                  = "EXTI4_IRQn",             /*!< EXTI Line4 Interrupt                                              */
  [DMA1_Stream0_IRQn]           = "DMA1_Stream0_IRQn",      /*!< DMA1 Stream 0 global Interrupt                                    */
  [DMA1_Stream1_IRQn]           = "DMA1_Stream1_IRQn",      /*!< DMA1 Stream 1 global Interrupt                                    */
  [DMA1_Stream2_IRQn]           = "DMA1_Stream2_IRQn",      /*!< DMA1 Stream 2 global Interrupt                                    */
  [DMA1_Stream3_IRQn]           = "DMA1_Stream3_IRQn",      /*!< DMA1 Stream 3 global Interrupt                                    */
  [DMA1_Stream4_IRQn]           = "DMA1_Stream4_IRQn",      /*!< DMA1 Stream 4 global Interrupt                                    */
  [DMA1_Stream5_IRQn]           = "DMA1_Stream5_IRQn",      /*!< DMA1 Stream 5 global Interrupt                                    */
  [DMA1_Stream6_IRQn]           = "DMA1_Stream6_IRQn",      /*!< DMA1 Stream 6 global Interrupt                                    */
  [ADC_IRQn]                    = "ADC_IRQn",               /*!< ADC1 and  ADC2 global Interrupts                                  */
  [FDCAN1_IT0_IRQn]             = "FDCAN1_IT0_IRQn",        /*!< FDCAN1 Interrupt line 0                                           */
  [FDCAN2_IT0_IRQn]             = "FDCAN2_IT0_IRQn",        /*!< FDCAN2 Interrupt line 0                                           */
  [FDCAN1_IT1_IRQn]             = "FDCAN1_IT1_IRQn",        /*!< FDCAN1 Interrupt line 1                                           */
  [FDCAN2_IT1_IRQn]             = "FDCAN2_IT1_IRQn",        /*!< FDCAN2 Interrupt line 1                                           */
  [EXTI9_5_IRQn]                = "EXTI9_5_IRQn",           /*!< External Line[9:5] Interrupts                                     */
  [TIM1_BRK_IRQn]               = "TIM1_BRK_IRQn",          /*!< TIM1 Break Interrupt                                              */
  [TIM1_UP_IRQn]                = "TIM1_UP_IRQn",           /*!< TIM1 Update Interrupt                                             */
  [TIM1_TRG_COM_IRQn]           = "TIM1_TRG_COM_IRQn",      /*!< TIM1 Trigger and Commutation Interrupt                            */
  [TIM1_CC_IRQn]                = "TIM1_CC_IRQn",           /*!< TIM1 Capture Compare Interrupt                                    */
  [TIM2_IRQn]                   = "TIM2_IRQn",              /*!< TIM2 global Interrupt                                             */
  [TIM3_IRQn]                   = "TIM3_IRQn",              /*!< TIM3 global Interrupt                                             */
  [TIM4_IRQn]                   = "TIM4_IRQn",              /*!< TIM4 global Interrupt                                             */
  [I2C1_EV_IRQn]                = "I2C1_EV_IRQn",           /*!< I2C1 Event Interrupt                                              */
  [I2C1_ER_IRQn]                = "I2C1_ER_IRQn",           /*!< I2C1 Error Interrupt                                              */
  [I2C2_EV_IRQn]                = "I2C2_EV_IRQn",           /*!< I2C2 Event Interrupt                                              */
  [I2C2_ER_IRQn]                = "I2C2_ER_IRQn",           /*!< I2C2 Error Interrupt                                              */
  [SPI1_IRQn]                   = "SPI1_IRQn",              /*!< SPI1 global Interrupt                                             */
  [SPI2_IRQn]                   = "SPI2_IRQn",              /*!< SPI2 global Interrupt                                             */
  [USART1_IRQn]                 = "USART1_IRQn",            /*!< USART1 global Interrupt                                           */
  [USART2_IRQn]                 = "USART2_IRQn",            /*!< USART2 global Interrupt                                           */
  [USART3_IRQn]                 = "USART3_IRQn",            /*!< USART3 global Interrupt                                           */
  [EXTI15_10_IRQn]              = "EXTI15_10_IRQn",         /*!< External Line[15:10] Interrupts                                   */
  [RTC_Alarm_IRQn]              = "RTC_Alarm_IRQn",         /*!< RTC Alarm (A and B) through EXTI Line Interrupt                   */
  [TIM8_BRK_TIM12_IRQn]         = "TIM8_BRK_TIM12_IRQn",    /*!< TIM8 Break Interrupt and TIM12 global interrupt                   */
  [TIM8_UP_TIM13_IRQn]          = "TIM8_UP_TIM13_IRQn",     /*!< TIM8 Update Interrupt and TIM13 global interrupt                  */
  [TIM8_TRG_COM_TIM14_IRQn]     = "TIM8_TRG_COM_TIM14_IRQn",/*!< TIM8 Trigger and Commutation Interrupt and TIM14 global interrupt */
  [TIM8_CC_IRQn]                = "TIM8_CC_IRQn",           /*!< TIM8 Capture Compare Interrupt                                    */
  [DMA1_Stream7_IRQn]           = "DMA1_Stream7_IRQn",      /*!< DMA1 Stream7 Interrupt                                            */
  [FMC_IRQn]                    = "FMC_IRQn",               /*!< FMC global Interrupt                                              */
  [SDMMC1_IRQn]                 = "SDMMC1_IRQn",            /*!< SDMMC1 global Interrupt                                           */
  [TIM5_IRQn]                   = "TIM5_IRQn",              /*!< TIM5 global Interrupt                                             */
  [SPI3_IRQn]                   = "SPI3_IRQn",              /*!< SPI3 global Interrupt                                             */
  [UART4_IRQn]                  = "UART4_IRQn",             /*!< UART4 global Interrupt                                            */
  [UART5_IRQn]                  = "UART5_IRQn",             /*!< UART5 global Interrupt                                            */
  [TIM6_DAC_IRQn]               = "TIM6_DAC_IRQn",          /*!< TIM6 global and DAC1&2 underrun error  interrupts                 */
  [TIM7_IRQn]                   = "TIM7_IRQn",              /*!< TIM7 global interrupt                                             */
  [DMA2_Stream0_IRQn]           = "DMA2_Stream0_IRQn",      /*!<   DMA2 Stream 0 global Interrupt                                  */
  [DMA2_Stream1_IRQn]           = "DMA2_Stream1_IRQn",      /*!<   DMA2 Stream 1 global Interrupt                                  */
  [DMA2_Stream2_IRQn]           = "DMA2_Stream2_IRQn",      /*!<   DMA2 Stream 2 global Interrupt                                  */
  [DMA2_Stream3_IRQn]           = "DMA2_Stream3_IRQn",      /*!<   DMA2 Stream 3 global Interrupt                                  */
  [DMA2_Stream4_IRQn]           = "DMA2_Stream4_IRQn",      /*!<   DMA2 Stream 4 global Interrupt                                  */
  [ETH_IRQn]                    = "ETH_IRQn",               /*!< Ethernet global Interrupt                                         */
  [ETH_WKUP_IRQn]               = "ETH_WKUP_IRQn",          /*!< Ethernet Wakeup through EXTI line Interrupt                       */
  [FDCAN_CAL_IRQn]              = "FDCAN_CAL_IRQn",         /*!< FDCAN Calibration unit Interrupt                                  */
  [DMA2_Stream5_IRQn]           = "DMA2_Stream5_IRQn",      /*!< DMA2 Stream 5 global interrupt                                    */
  [DMA2_Stream6_IRQn]           = "DMA2_Stream6_IRQn",      /*!< DMA2 Stream 6 global interrupt                                    */
  [DMA2_Stream7_IRQn]           = "DMA2_Stream7_IRQn",      /*!< DMA2 Stream 7 global interrupt                                    */
  [USART6_IRQn]                 = "USART6_IRQn",            /*!< USART6 global interrupt                                           */
  [I2C3_EV_IRQn]                = "I2C3_EV_IRQn",           /*!< I2C3 event interrupt                                              */
  [I2C3_ER_IRQn]                = "I2C3_ER_IRQn",           /*!< I2C3 error interrupt                                              */
  [OTG_HS_EP1_OUT_IRQn]         = "OTG_HS_EP1_OUT_IRQn",    /*!< USB OTG HS End Point 1 Out global interrupt                       */
  [OTG_HS_EP1_IN_IRQn]          = "OTG_HS_EP1_IN_IRQn",     /*!< USB OTG HS End Point 1 In global interrupt                        */
  [OTG_HS_WKUP_IRQn]            = "OTG_HS_WKUP_IRQn",       /*!< USB OTG HS Wakeup through EXTI interrupt                          */
  [OTG_HS_IRQn]                 = "OTG_HS_IRQn",            /*!< USB OTG HS global interrupt                                       */
  [DCMI_IRQn]                   = "DCMI_IRQn",              /*!< DCMI global interrupt                                             */
  [CRYP_IRQn]                   = "CRYP_IRQn",              /*!< CRYP crypto global interrupt                                      */
  [HASH_RNG_IRQn]               = "HASH_RNG_IRQn",          /*!< HASH and RNG global interrupt                                     */
  [FPU_IRQn]                    = "FPU_IRQn",               /*!< FPU global interrupt                                              */
  [UART7_IRQn]                  = "UART7_IRQn",             /*!< UART7 global interrupt                                            */
  [UART8_IRQn]                  = "UART8_IRQn",             /*!< UART8 global interrupt                                            */
  [SPI4_IRQn]                   = "SPI4_IRQn",              /*!< SPI4 global Interrupt                                             */
  [SPI5_IRQn]                   = "SPI5_IRQn",              /*!< SPI5 global Interrupt                                             */
  [SPI6_IRQn]                   = "SPI6_IRQn",              /*!< SPI6 global Interrupt                                             */
  [SAI1_IRQn]                   = "SAI1_IRQn",              /*!< SAI1 global Interrupt                                             */
  [LTDC_IRQn]                   = "LTDC_IRQn",              /*!< LTDC global Interrupt                                             */
  [LTDC_ER_IRQn]                = "LTDC_ER_IRQn",           /*!< LTDC Error global Interrupt                                       */
  [DMA2D_IRQn]                  = "DMA2D_IRQn",             /*!< DMA2D global Interrupt                                            */
  [SAI2_IRQn]                   = "SAI2_IRQn",              /*!< SAI2 global Interrupt                                             */
  [QUADSPI_IRQn]                = "QUADSPI_IRQn",           /*!< Quad SPI global interrupt                                         */
  [LPTIM1_IRQn]                 = "LPTIM1_IRQn",            /*!< LP TIM1 interrupt                                                 */
  [CEC_IRQn]                    = "CEC_IRQn",               /*!< HDMI-CEC global Interrupt                                         */
  [I2C4_EV_IRQn]                = "I2C4_EV_IRQn",           /*!< I2C4 Event Interrupt                                              */
  [I2C4_ER_IRQn]                = "I2C4_ER_IRQn",           /*!< I2C4 Error Interrupt                                              */
  [SPDIF_RX_IRQn]               = "SPDIF_RX_IRQn",          /*!< SPDIF-RX global Interrupt                                         */
  [OTG_FS_EP1_OUT_IRQn]         = "OTG_FS_EP1_OUT_IRQn",    /*!< USB OTG HS2 global interrupt                                      */
  [OTG_FS_EP1_IN_IRQn]          = "OTG_FS_EP1_IN_IRQn",     /*!< USB OTG HS2 End Point 1 Out global interrupt                      */
  [OTG_FS_WKUP_IRQn]            = "OTG_FS_WKUP_IRQn",       /*!< USB OTG HS2 End Point 1 In global interrupt                       */
  [OTG_FS_IRQn]                 = "OTG_FS_IRQn",            /*!< USB OTG HS2 Wakeup through EXTI interrupt                         */
  [DMAMUX1_OVR_IRQn]            = "DMAMUX1_OVR_IRQn",       /*!<DMAMUX1 Overrun interrupt                                          */
  [HRTIM1_Master_IRQn]          = "HRTIM1_Master_IRQn",     /*!< HRTIM Master Timer global Interrupts                              */
  [HRTIM1_TIMA_IRQn]            = "HRTIM1_TIMA_IRQn",       /*!< HRTIM Timer A global Interrupt                                    */
  [HRTIM1_TIMB_IRQn]            = "HRTIM1_TIMB_IRQn",       /*!< HRTIM Timer B global Interrupt                                    */
  [HRTIM1_TIMC_IRQn]            = "HRTIM1_TIMC_IRQn",       /*!< HRTIM Timer C global Interrupt                                    */
  [HRTIM1_TIMD_IRQn]            = "HRTIM1_TIMD_IRQn",       /*!< HRTIM Timer D global Interrupt                                    */
  [HRTIM1_TIME_IRQn]            = "HRTIM1_TIME_IRQn",       /*!< HRTIM Timer E global Interrupt                                    */
  [HRTIM1_FLT_IRQn]             = "HRTIM1_FLT_IRQn",        /*!< HRTIM Fault global Interrupt                                      */
  [DFSDM1_FLT0_IRQn]            = "DFSDM1_FLT0_IRQn",       /*!<DFSDM Filter1 Interrupt                                            */
  [DFSDM1_FLT1_IRQn]            = "DFSDM1_FLT1_IRQn",       /*!<DFSDM Filter2 Interrupt                                            */
  [DFSDM1_FLT2_IRQn]            = "DFSDM1_FLT2_IRQn",       /*!<DFSDM Filter3 Interrupt                                            */
  [DFSDM1_FLT3_IRQn]            = "DFSDM1_FLT3_IRQn",       /*!<DFSDM Filter4 Interrupt                                            */
  [SAI3_IRQn]                   = "SAI3_IRQn",              /*!< SAI3 global Interrupt                                             */
  [SWPMI1_IRQn]                 = "SWPMI1_IRQn",            /*!< Serial Wire Interface 1 global interrupt                          */
  [TIM15_IRQn]                  = "TIM15_IRQn",             /*!< TIM15 global Interrupt                                            */
  [TIM16_IRQn]                  = "TIM16_IRQn",             /*!< TIM16 global Interrupt                                            */
  [TIM17_IRQn]                  = "TIM17_IRQn",             /*!< TIM17 global Interrupt                                            */
  [MDIOS_WKUP_IRQn]             = "MDIOS_WKUP_IRQn",        /*!< MDIOS Wakeup  Interrupt                                           */
  [MDIOS_IRQn]                  = "MDIOS_IRQn",             /*!< MDIOS global Interrupt                                            */
  [JPEG_IRQn]                   = "JPEG_IRQn",              /*!< JPEG global Interrupt                                             */
  [MDMA_IRQn]                   = "MDMA_IRQn",              /*!< MDMA global Interrupt                                             */
  [SDMMC2_IRQn]                 = "SDMMC2_IRQn",            /*!< SDMMC2 global Interrupt                                           */
  [HSEM1_IRQn]                  = "HSEM1_IRQn",             /*!< HSEM1 global Interrupt                                            */
  [ADC3_IRQn]                   = "ADC3_IRQn",              /*!< ADC3 global Interrupt                                             */
  [DMAMUX2_OVR_IRQn]            = "DMAMUX2_OVR_IRQn",       /*!<DMAMUX2 Overrun interrupt                                          */
  [BDMA_Channel0_IRQn]          = "BDMA_Channel0_IRQn",     /*!< BDMA Channel 0 global Interrupt                                   */
  [BDMA_Channel1_IRQn]          = "BDMA_Channel1_IRQn",     /*!< BDMA Channel 1 global Interrupt                                   */
  [BDMA_Channel2_IRQn]          = "BDMA_Channel2_IRQn",     /*!< BDMA Channel 2 global Interrupt                                   */
  [BDMA_Channel3_IRQn]          = "BDMA_Channel3_IRQn",     /*!< BDMA Channel 3 global Interrupt                                   */
  [BDMA_Channel4_IRQn]          = "BDMA_Channel4_IRQn",     /*!< BDMA Channel 4 global Interrupt                                   */
  [BDMA_Channel5_IRQn]          = "BDMA_Channel5_IRQn",     /*!< BDMA Channel 5 global Interrupt                                   */
  [BDMA_Channel6_IRQn]          = "BDMA_Channel6_IRQn",     /*!< BDMA Channel 6 global Interrupt                                   */
  [BDMA_Channel7_IRQn]          = "BDMA_Channel7_IRQn",     /*!< BDMA Channel 7 global Interrupt                                   */
  [COMP_IRQn]                   = "COMP_IRQn" ,             /*!< COMP global Interrupt                                             */
  [LPTIM2_IRQn]                 = "LPTIM2_IRQn",            /*!< LP TIM2 global interrupt                                          */
  [LPTIM3_IRQn]                 = "LPTIM3_IRQn",            /*!< LP TIM3 global interrupt                                          */
  [LPTIM4_IRQn]                 = "LPTIM4_IRQn",            /*!< LP TIM4 global interrupt                                          */
  [LPTIM5_IRQn]                 = "LPTIM5_IRQn",            /*!< LP TIM5 global interrupt                                          */
  [LPUART1_IRQn]                = "LPUART1_IRQn",           /*!< LP UART1 interrupt                                                */
  [CRS_IRQn]                    = "CRS_IRQn",               /*!< Clock Recovery Global Interrupt                                   */
  [ECC_IRQn]                    = "ECC_IRQn",               /*!< ECC diagnostic Global Interrupt                                   */
  [SAI4_IRQn]                   = "SAI4_IRQn",              /*!< SAI4 global interrupt                                             */
  [WAKEUP_PIN_IRQn]             = "WAKEUP_PIN_IRQn",        /*!< Interrupt for all 6 wake-up pins                                  */
};
/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  获取NVIC优先级.
  * @param  None.
  * @retval None.
  * @note   None.
*/
void nvic_irq_get(void)
{
  rt_kprintf("ldx name                 ");
  rt_kprintf("E P A Priotity\n");
  for (rt_uint8_t i = 0; i < 255; i++)
  {
      if(NVIC_GetEnableIRQ(i))
      {
          rt_kprintf("%3d ",i);
          rt_kprintf("%-20.20s",nvic_name[i]);
          NVIC_GetPendingIRQ(i) ? rt_kprintf(" 1") : rt_kprintf(" 0");
          NVIC_GetActive(i) ? rt_kprintf(" 1") : rt_kprintf(" 0");
          rt_kprintf(" %02d\n",NVIC_GetPriority(i));
      }
  }
}
MSH_CMD_EXPORT(nvic_irq_get, get all enable NVIC_IRQ);