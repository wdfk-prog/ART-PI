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

/* Private includes ----------------------------------------------------------*/
#include <rtthread.h>
#include "board.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
#define NAME_LEN 30
/* Private variables ---------------------------------------------------------*/
static const char * const nvic_name[] = {
#if defined(SOC_SERIES_STM32H7)
    #include "inc/irq_stm32h7.h"
#else
    #error "Unsupported chips"
#endif
};
/* Private function prototypes -----------------------------------------------*/
rt_inline void object_split(int len)
{
    while (len--) rt_kprintf(" ");
}
/**
  * @brief  获取NVIC优先级.
  * @param  None.
  * @retval None.
  * @note   None.
*/
void nvic_irq_get(void)
{
  rt_kprintf("ldx name");
  object_split(NAME_LEN - 3);
  rt_kprintf("E P A Priotity\n");
  for (rt_uint8_t i = 0; i < sizeof(nvic_name) / sizeof(nvic_name[1]); i++)
  {
      if(NVIC_GetEnableIRQ(i))
      {
          rt_kprintf("%3d ",i);
          rt_kprintf("%-*.*s 1",NAME_LEN,NAME_LEN,nvic_name[i]);
          NVIC_GetPendingIRQ(i) ? rt_kprintf(" 1") : rt_kprintf(" 0");
          NVIC_GetActive(i)     ? rt_kprintf(" 1") : rt_kprintf(" 0");
          rt_kprintf("    %02d\n",NVIC_GetPriority(i));
      }
  }
}
MSH_CMD_EXPORT(nvic_irq_get, get all enable NVIC_IRQ);
