/**
 * @file user_adc.c
 * @brief 
 * @author HLY (1425075683@qq.com)
 * @version 1.0
 * @date 2022-12-20
 * @copyright Copyright (c) 2022
 * @attention 
 * @par 修改日志:
 * Date       Version Author  Description
 * 2022-12-20 1.0     HLY     first version
 */
/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
#include <rtthread.h>
#include <rtdevice.h>
#include <main.h>
#include <dma.h>
/* Private typedef -----------------------------------------------------------*/
/** 
  * @brief ADC1通道值定义
  */  
typedef enum
{
  VREFINT_CHANNEL = 0x00,
  TEMP_CHANNEL,
}ADC1_CHANNEL;
/* Private define ------------------------------------------------------------*/
#define ADC3_CHANNEL_NUM 2  //ADC3用了2通道
#define ADC3_BUFFER_SIZE 8 //每个通道存32组, 方便做平均
#define ADC3_BUFFER_LEN  ADC3_BUFFER_SIZE * ADC3_CHANNEL_NUM
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern ADC_HandleTypeDef hadc3;
uint16_t adc3_data[ADC3_BUFFER_LEN];
/* Private function prototypes -----------------------------------------------*/
extern void MX_ADC3_Init(void);
/**
  * @brief  ADC初始化
  * @param  None
  * @retval int 
  * @note   None
*/
void adc_init(void)
{
    /* DMA1_Stream3_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    MX_ADC3_Init();
  
    /* 校准 ADC，采用偏移校准 */ 
    if (HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK) 
    { 
      Error_Handler();
    }
    if (HAL_ADC_Start_DMA(&hadc3, (uint32_t *)adc3_data, ADC3_BUFFER_LEN) != HAL_OK)
    {
        Error_Handler();
    }
}
/**
  * @brief  ADC 测试命令函数
  * @param  None
  * @retval int 
  * @note   None
*/
static int adc_vol_sample(void)
{
    rt_uint16_t vref_mv,temp_mv; 
    rt_uint32_t vref_value,temp_value;
    rt_uint32_t sum;
  
    for(int i = VREFINT_CHANNEL; i < ADC3_BUFFER_LEN; i += ADC3_CHANNEL_NUM)
    {
      sum += adc3_data[i];
    }
    vref_value = sum / ADC3_BUFFER_SIZE;
    sum = 0;
    for(int i = TEMP_CHANNEL; i < ADC3_BUFFER_LEN; i += ADC3_CHANNEL_NUM)
    {
      sum += adc3_data[i];
    }
    temp_value = sum / ADC3_BUFFER_SIZE;
    
    rt_kprintf("Vref  is %u.\n", vref_value);
    rt_kprintf("Temp is %u.\n" , temp_value);
    // Calculating Vref voltage
    vref_mv = __HAL_ADC_CALC_VREFANALOG_VOLTAGE(vref_value, ADC_RESOLUTION_16B);
    rt_kprintf("Vref voltage is %u mV.\n", vref_mv);
    // Calculate Temperature
    rt_kprintf("%d are Temperature in Degree C.\n",
    __HAL_ADC_CALC_TEMPERATURE(vref_mv, temp_value, ADC_RESOLUTION_16B));
    return RT_EOK;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(adc_vol_sample, adc voltage convert sample);
/**
  * @brief  ADC_DMA 半满回调函数
  * @param  None
  * @retval None
  * @note   ADC转换半满中断中把数据存到数组的前半部分
            ping-pong存储
            FIFO
*/
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer */
  if(hadc->Instance == ADC3) 
  {
      SCB_InvalidateDCache_by_Addr((uint32_t *) &adc3_data[0], ADC3_BUFFER_LEN);
  }
}
/**
  * @brief  ADC_DMA 转换完成回调函数
  * @param  None
  * @retval None
  * @note   ADC转换完成中断中把数据存到数组的后半部分
*/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
   /* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer */
   if(hadc->Instance == ADC3) 
   {
       SCB_InvalidateDCache_by_Addr((uint32_t *) &adc3_data[ADC3_BUFFER_LEN/2], ADC3_BUFFER_LEN);
   }
}