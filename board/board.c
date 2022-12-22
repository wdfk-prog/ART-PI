/**
 * @file board.c
 * @brief 
 * @author HLY (1425075683@qq.com)
 * @version 1.0
 * @date 2022-11-18
 * @copyright Copyright (c) 2022
 * @attention 
 * @par 修改日志:
 * Date       Version Author  Description
 * 2022-11-18 1.0     HLY     first version
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "board.h"
/* Private includes ----------------------------------------------------------*/
#include <time.h>
#include "main.h"
#ifdef RT_USING_SERIAL
#ifdef RT_USING_SERIAL_V2
#include "drv_usart_v2.h"
#else
#include "drv_usart.h"
#endif /* RT_USING_SERIAL */
#endif /* RT_USING_SERIAL_V2 */
/*ulog include*/
#define LOG_TAG              "board" 
#define LOG_LVL              DBG_INFO
#include <ulog.h>
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define RTC_NAME       "rtc"
/*串口中断优先级设置*/
#define FINSH_IRQ_PRIORITY 3

#define PVD_ENABLE 0
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
#ifdef FINSH_IRQ_PRIORITY
/**
  * @brief  设置FINSH串口中断优先级
  * @param  None
  * @retval None
*/
static int set_finsh_irq(void)
{
  rt_err_t ret = RT_EOK;
  /* 串口设备句柄 */
  rt_device_t serial;
  /* 查找串口设备 */
  serial = rt_device_find(RT_CONSOLE_DEVICE_NAME);
  if (!serial)
  {
      LOG_E("find %s failed!", RT_CONSOLE_DEVICE_NAME);
      ret = -RT_ERROR;
  }
  struct stm32_uart *uart;

  uart = rt_container_of(serial, struct stm32_uart, serial);
  /* parameter check */
  RT_ASSERT(uart != RT_NULL);
  HAL_NVIC_SetPriority(uart->config->irq_type,FINSH_IRQ_PRIORITY, 0);
  return ret;
}
INIT_COMPONENT_EXPORT(set_finsh_irq);
#endif /*FINSH_IRQ_PRIORITY*/
/**
  * @brief  内部QSPI_FLASH中的向量表重定位
  * @param  None
  * @retval None
  */
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);
/**
  * @brief  获取嵌套向量中断控制器中断请求.
  * @param  None.
  * @retval None.
  * @note   None.
*/
void get_nvic_irq(void)
{
  rt_kprintf("ldx E P A Priotity\n");
  for (rt_uint8_t i = 0; i < 255; i++)
  {
      if(NVIC_GetEnableIRQ(i))
      {
          rt_kprintf("%3d 1",i);
          NVIC_GetPendingIRQ(i) ? rt_kprintf(" 1") : rt_kprintf(" 0");
          NVIC_GetActive(i) ? rt_kprintf(" 1") : rt_kprintf(" 0");
          rt_kprintf(" %02d\n",NVIC_GetPriority(i));
      }
  }
}
MSH_CMD_EXPORT(get_nvic_irq, get all enable NVIC_IRQ);
/**
 * @brief  获取编译时间戳
 * @param  None.
 * @retval time_t 返回时间戳
 * @note   None.
 */
static time_t rtc_get_build(void)
{
  struct tm tm_new = {0};
  /* update date. */
  tm_new.tm_year  = YEAR - 1900;// years since 1900
  tm_new.tm_mon   = MONTH;//tm_mon: 0~11
  tm_new.tm_mday  = DAY;
  /* update time. */
  tm_new.tm_hour = HOUR;
  tm_new.tm_min  = MINUTE;
  tm_new.tm_sec  = SEC;

  return mktime(&tm_new);
}
/**
 * @brief   rtc_update_init
 * @retval  int 
 * @note    初始化更新rtc时间
 */
int rtc_update_init(void)
{
    rt_thread_t tid;
    rt_err_t ret = RT_EOK;
    time_t now,build;

    rt_device_t device = RT_NULL;
    /*寻找设备*/
    device = rt_device_find(RTC_NAME);
    if (!device)
    {
        LOG_E("find %s failed!", RTC_NAME);
        return RT_ERROR;
    }
    /*初始化RTC设备*/
    if(rt_device_open(device, 0) != RT_EOK)
    {
        LOG_E("open %s failed!", RTC_NAME);
        return RT_ERROR;
    }
    
    build = rtc_get_build();//读取编译时间
    set_timestamp(build);

    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rtc_update_init);
/*********************************掉电检测******************************************/
#if (PVD_ENABLE == 1)
/* 完成量控制块 */
static struct rt_completion pvd_completion;
/**
  * @brief  None.
  * @param  None.
  * @retval None.
  * @note   None.
*/
static void pvd_thread_entry(void* parameter)
{
  while(1)
  {
      rt_completion_wait(&pvd_completion, RT_WAITING_FOREVER);
      /* 掉电前的紧急处理 */
      ulog_flush();
      rt_kprintf("Flush ULOG buffer complete\n");
  }
}
/**
 * @brief pvd初始化
 * @retval int 
 */
static int pvd_init(void)
{
    /*##-1- Enable Power Clock #################################################*/
 
    /*##-2- Configure the NVIC for PVD #########################################*/
    HAL_NVIC_SetPriority(PVD_IRQn, 0, 0); /* 配置PVD中断优先级 */
    HAL_NVIC_EnableIRQ(PVD_IRQn);         /* 使能PVD中断 */
 
    /* Configure the PVD Level to 3 and generate an interrupt on rising and falling
       edges(PVD detection level set to 2.5V, refer to the electrical characteristics
       of you device datasheet for more details) */
    PWR_PVDTypeDef sConfigPVD;
    sConfigPVD.PVDLevel = PWR_PVDLEVEL_6;     /* PVD阈值3.1V */
    sConfigPVD.Mode = PWR_PVD_MODE_IT_RISING; /* 检测掉电 */
    HAL_PWR_ConfigPVD(&sConfigPVD);
 
    /* Enable the PVD Output */
    HAL_PWR_EnablePVD();

    /* 初始化完成量对象 */
    rt_completion_init(&pvd_completion);
    rt_thread_t tid;
    tid = rt_thread_create("PVD", pvd_thread_entry, RT_NULL,
                          512, 0, 20);
    if(tid == RT_NULL)
    {
      LOG_E("PVD thread start failed!");
    }
    else
    {
      rt_thread_startup(tid);
    }
    return RT_EOK;
}
INIT_APP_EXPORT(pvd_init);
/**
  * @brief  PWR PVD interrupt callback
  * @retval None
  */
void HAL_PWR_PVDCallback(void)
{
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_PWR_PVDCallback could be implemented in the user file
   */
   if(__HAL_PWR_GET_FLAG( PWR_FLAG_PVDO ))    /* 1为VDD小于PVD阈值,掉电情况 */
  {
      rt_completion_done(&pvd_completion);
      ulog_i("PVD","Voltage below 3.1V was detected");
  }
}
#endif /*(PVD_ENABLE == 1)*/