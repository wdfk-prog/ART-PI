#include <main.h>

extern ADC_HandleTypeDef hadc3;

static void adc_vol_sample(void)
{
    rt_uint32_t vref_value,temp_value;
    rt_uint16_t vref_mv,temp_mv; 
  
    // Reading ADC3 inputs VBAT, VRef, and Internal Temperature Sensor Channel.
    if( HAL_OK != HAL_ADC_Start(&hadc3))
    {
      Error_Handler();
    }
    
    if(HAL_OK != HAL_ADC_PollForConversion(&hadc3, 100))
    {
      Error_Handler();
    }
    vref_value = HAL_ADC_GetValue(&hadc3);
    
    // Measure Temperature
    if(HAL_OK != HAL_ADC_PollForConversion(&hadc3, 100))
    {
      Error_Handler();
    }
    temp_value = HAL_ADC_GetValue(&hadc3);
    
    HAL_ADC_Stop(&hadc3);
    
    rt_kprintf("VRef  is %u.\n", vref_value);
    rt_kprintf("Temp is %u.\n" , temp_value);
    
    // Calculating VRef voltage
    vref_mv = __HAL_ADC_CALC_VREFANALOG_VOLTAGE(vref_value, ADC_RESOLUTION_16B);
    rt_kprintf("VRef voltage is %u mV.\n", vref_mv);
    // Calculate battery voltage
    rt_kprintf("%ld, and %d are Temperature in Degree C calculated using two different formulas.\n",
    __HAL_ADC_CALC_TEMPERATURE(vref_mv, temp_value, ADC_RESOLUTION_16B), 
    __HAL_ADC_CALC_TEMPERATURE_TYP_PARAMS(2000, 620, 30, vref_mv, temp_value, ADC_RESOLUTION_16B));
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(adc_vol_sample, adc voltage convert sample);