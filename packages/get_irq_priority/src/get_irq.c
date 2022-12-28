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
typedef struct
{
  rt_uint8_t ldx;       //IRQ编号
  rt_uint8_t priotity;  //优先级
}type;
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
#define NAME_LEN 30
/* Private variables ---------------------------------------------------------*/
static const char * const nvic_name[] = {
#if defined(SOC_SERIES_STM32H7)
    #include "inc/irq_stm32h7.h"
#elif defined(SOC_SERIES_STM32F4)
    #include "inc/irq_stm32f4.h"
#else
    #error "Unsupported chips"
#endif
};
#define IRQ_LEN sizeof(nvic_name) / sizeof(nvic_name[1])
/* Private function prototypes -----------------------------------------------*/
static void object_split(int len)
{
    while (len--) rt_kprintf(" ");
}
/**
  * @brief  获取NVIC优先级.
  * @param  None.
  * @retval None.
  * @note   None.
*/
static void nvic_irq_get(rt_uint8_t i)
{
    /* see:Properties of the different exception types (continued)
       Exception number(ldx) = 16 and above
    */
    rt_kprintf("%3d ",i + 16);
    rt_kprintf("%-*.*s 1",NAME_LEN,NAME_LEN,nvic_name[i]);
    NVIC_GetPendingIRQ(i) ? rt_kprintf(" 1") : rt_kprintf(" 0");
    NVIC_GetActive(i)     ? rt_kprintf(" 1") : rt_kprintf(" 0");
    rt_kprintf("    %02d\n",NVIC_GetPriority(i));
}
/**
  * @brief  获取NVIC优先级,以中断编号排序.
  * @param  None.
  * @retval None.
  * @note   None.
*/
static void nvic_irq_get_idx(void)
{
    rt_kprintf("ldx name");
    object_split(NAME_LEN - 3);
    rt_kprintf("E P A Priotity\n");
    for (rt_uint8_t i = 0; i < IRQ_LEN; i++)
    {
        if(NVIC_GetEnableIRQ(i))
        {
            nvic_irq_get(i);
        }
    }
}
/**
  * @brief  获取NVIC优先级,以中断优先级从低到高排序.
  * @param  None.
  * @retval None.
  * @note   None.
*/
static void nvic_irq_get_priotity(void)
{
    type buff[IRQ_LEN];
    rt_uint8_t temp;
    rt_memset(buff,0,sizeof(buff));

    for (rt_uint8_t i = 0; i < IRQ_LEN; i++)
    {
        if(NVIC_GetEnableIRQ(i))
        {
          buff[i].ldx      = i;
          buff[i].priotity = NVIC_GetPriority(i) + 1;//+1排除未使能0优先级
        }
    }
    //排序
    for (rt_uint8_t i = 0; i < IRQ_LEN - 1; i++)
    {
        for (rt_uint8_t j = 0; j < IRQ_LEN - 1 - i; j++)
        {
            if (buff[j].priotity > buff[j + 1].priotity) 
            {
                temp = buff[j].priotity;
                buff[j].priotity = buff[j + 1].priotity;
                buff[j + 1].priotity = temp;

                temp = buff[j].ldx;
                buff[j].ldx = buff[j + 1].ldx;
                buff[j + 1].ldx = temp;
            }
        }
    }

    rt_kprintf("ldx name");
    object_split(NAME_LEN - 3);
    rt_kprintf("E P A Priotity\n");
    for (rt_uint8_t i = 0; i < IRQ_LEN; i++)
    {
        if(buff[i].priotity)
        {
          nvic_irq_get(buff[i].ldx);
        }
    }
}
#ifdef RT_USING_MSH
/**
  * @brief  MSH命令
  * @param  None
  * @retval None
  * @note   None
*/
static void nvic_irq_msh(uint8_t argc, char **argv) 
{
#define IRQ_CMD_IDX               0
#define IRQ_CMD_PRIOTITY          1
#define MOTOR_CMD_SET             2

    size_t i = 0;

    const char* help_info[] =
    {
            [IRQ_CMD_IDX]             = "nvic_irq idx        - Get all enable NVIC_IRQ,sort by interrupt number.",
            [IRQ_CMD_PRIOTITY]        = "nvic_irq priority   - Get all enable NVIC_IRQ,sort by interrupt priority from low to high.",
            [MOTOR_CMD_SET]           = "nvic_irq set        - Sets the NVIC IRQ level.",
    };

    if (argc < 2)
    {
        nvic_irq_get_idx();
    }
    else
    {
        const char *operator = argv[1];

        if (!rt_strcmp(operator, "idx"))
        {
            nvic_irq_get_idx();
        }
        else if (!rt_strcmp(operator, "priority"))
        {
            nvic_irq_get_priotity();
        }
        else if (!rt_strcmp(operator, "set"))
        {
            return;
        }
        else
        {
            rt_kprintf("Usage:\n");
            for (i = 0; i < sizeof(help_info) / sizeof(char*); i++)
            {
                rt_kprintf("%s\n", help_info[i]);
            }
            rt_kprintf("\n");
        }
    }
}
MSH_CMD_EXPORT_ALIAS(nvic_irq_msh,nvic_irq,nvic_irq command.);
#endif /*RT_USING_MSH*/