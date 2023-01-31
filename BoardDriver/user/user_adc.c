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
/*ulog include*/
#define LOG_TAG              "measure" 
#define LOG_LVL              DBG_INFO
#include <ulog.h>
/* Private typedef -----------------------------------------------------------*/
#ifdef USER_USING_ADC
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
#define ADC3_BUFFER_SIZE 8  //每个通道存32组, 方便做平均
#define ADC3_BUFFER_LEN  ADC3_BUFFER_SIZE * ADC3_CHANNEL_NUM

#define REAL_TIME_REFRESH 0 //是否实时刷新缓存区
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern ADC_HandleTypeDef hadc3;
rt_uint16_t* adc3_data = RT_NULL;
/* Private function prototypes -----------------------------------------------*/
extern void MX_ADC3_Init(void);
/**
  * @brief  ADC初始化
  * @param  None
  * @retval int 
  * @note   None
*/
static int adc_init(void)
{
    /* DMA controller clock enable */
    __HAL_RCC_DMA1_CLK_ENABLE();
    /* DMA1_Stream3_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 15, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    MX_ADC3_Init();
 
    /* 校准 ADC，采用偏移校准 */ 
    if (HAL_ADCEx_Calibration_Start(&hadc3, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK) 
    { 
      Error_Handler();
    }

    adc3_data = (rt_uint16_t *)rt_malloc_align(ADC3_BUFFER_LEN,16);
    
    if (HAL_ADC_Start_DMA(&hadc3, (uint32_t *)adc3_data, ADC3_BUFFER_LEN) != HAL_OK)
    {
        Error_Handler();
    }
    
    return RT_EOK;
}
INIT_DEVICE_EXPORT(adc_init);
/**
  * @brief  获取 MCU内部参考电压和温度
  * @param  None
  * @retval int 
  * @note   None
*/
int vref_temp_get(void)
{
    rt_uint16_t vref_mv,temp_mv; 
    rt_uint32_t vref_value,temp_value;
    rt_uint32_t sum = 0;
#if (REAL_TIME_REFRESH == 0)
    //使用时才刷新缓存
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, adc3_data, ADC3_BUFFER_LEN);
#endif /* (REAL_TIME_REFRESH == 0) */
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
    
    LOG_I("Vref = %u, Temp = %u", vref_value, temp_value);
    
    // Calculating Vref voltage
    vref_mv = __HAL_ADC_CALC_VREFANALOG_VOLTAGE(vref_value, ADC_RESOLUTION_16B);
    LOG_I("Vref voltages = %u mV", vref_mv);
    
    // Calculate Temperature
    LOG_I("Temperature = %d Degree C",
    __HAL_ADC_CALC_TEMPERATURE(vref_mv, temp_value, ADC_RESOLUTION_16B));
    return RT_EOK;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(vref_temp_get, get MCU vref and temp);
#if (REAL_TIME_REFRESH == 0)
/**
  * @brief  ADC_DMA 半满回调函数
  * @param  hadc
  * @retval None
  * @note   ADC转换半满中断中把数据存到数组的前半部分
            ping-pong存储
*/
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    /* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer */
    if(hadc->Instance == ADC3) 
    {
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, adc3_data, ADC3_BUFFER_LEN);
    }
}
/**
  * @brief  ADC_DMA 转换完成回调函数
  * @param  hadc
  * @retval None
  * @note   ADC转换完成中断中把数据存到数组的后半部分
*/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    /* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer */
    if(hadc->Instance == ADC3) 
    {
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, adc3_data + ADC3_BUFFER_LEN / 2, ADC3_BUFFER_LEN);
    }
}
#endif /* (REAL_TIME_REFRESH == 0) */
#else
#define ADC_DEV_NAME        "adc3"      /* ADC 设备名称 */
#define ADC_VREF_CHANNEL     ADC_CHANNEL_VREFINT - ADC_CHANNEL_0    /* ADC Vref 通道 */
#define ADC_TEMP_CHANNEL     ADC_CHANNEL_TEMPSENSOR - ADC_CHANNEL_0 /* ADC Temp 通道 */
/**
  * @brief  获取 MCU内部参考电压和温度
  * @param  None
  * @retval int 
  * @note   None
*/
int vref_temp_get(void)
{
    rt_uint32_t vref_value,temp_value;
    rt_uint16_t vref_mv,temp_mv; 

    rt_adc_device_t adc_dev;
    rt_err_t ret = RT_EOK;
    
    /* 查找设备 */
    adc_dev = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc_dev == RT_NULL)
    {
        rt_kprintf("adc sample run failed! can't find %s device!\n", ADC_DEV_NAME);
        return RT_ERROR;
    }

    /* 使能设备 */
    ret = rt_adc_enable(adc_dev, ADC_VREF_CHANNEL);
    /* 读取采样值 */
    vref_value = rt_adc_read(adc_dev, ADC_VREF_CHANNEL);
    /* 关闭通道 */
    ret = rt_adc_disable(adc_dev, ADC_VREF_CHANNEL);

    /* 使能设备 */
    ret = rt_adc_enable(adc_dev, ADC_TEMP_CHANNEL);
    /* 读取采样值 */
    temp_value = rt_adc_read(adc_dev, ADC_TEMP_CHANNEL);
    /* 关闭通道 */
    ret = rt_adc_disable(adc_dev, ADC_TEMP_CHANNEL);
  
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
MSH_CMD_EXPORT(vref_temp_get, get MCU vref and temp);
#endif /* USER_USING_ADC */