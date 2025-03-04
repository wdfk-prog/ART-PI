/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-7      SummerGift   first version
 */

#include "drv_common.h"
#include <board.h>
#include "gpio.h"

#ifdef RT_USING_PIN
#include <drv_gpio.h>
#endif

#ifdef RT_USING_SERIAL
#ifdef RT_USING_SERIAL_V2
#include <drv_usart_v2.h>
#else
#include <drv_usart.h>
#endif /* RT_USING_SERIAL */
#endif /* RT_USING_SERIAL_V2 */

#define DBG_TAG "drv_common"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#ifdef RT_USING_FINSH
#include <finsh.h>
static void reboot(uint8_t argc, char **argv)
{
    rt_hw_cpu_reset();
}
MSH_CMD_EXPORT(reboot, Reboot System);
#endif /* RT_USING_FINSH */

extern __IO uint32_t uwTick;
static uint32_t _systick_ms = 1;

/* SysTick configuration */
void rt_hw_systick_init(void)
{
    // Updates the variable SystemCoreClock
    SystemCoreClockUpdate();

    HAL_SYSTICK_Config(SystemCoreClock / RT_TICK_PER_SECOND);

    NVIC_SetPriority(SysTick_IRQn, 0xFF);

    _systick_ms = 1000u / RT_TICK_PER_SECOND;
    if (_systick_ms == 0)
        _systick_ms = 1;
}

/**
 * This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
        HAL_IncTick();

    rt_tick_increase();

    /* leave interrupt */
    rt_interrupt_leave();
}

uint32_t HAL_GetTick(void)
{
    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
        HAL_IncTick();

    return uwTick;
}

void HAL_IncTick(void)
{
    uwTick += _systick_ms;
}

void HAL_SuspendTick(void)
{
}

void HAL_ResumeTick(void)
{
}

void HAL_Delay(__IO uint32_t Delay)
{
    if (rt_thread_self())
    {
        rt_thread_mdelay(Delay);
    }
    else
    {
        for (rt_uint32_t count = 0; count < Delay; count++)
        {
            rt_hw_us_delay(1000);
        }
    }
}

void printf_Reset_Status(void)
{
    enum
    {
        CPURSTF     = 17,   //CPU 复位标志
        D1RSTF      = 19,   //D1 域电源开关复位标志
        D2RSTF      = 20,   //D2 域电源开关复位标志
        BORRSTF     = 21,   //BOR 复位标志
        PINRSTF     = 22,   //引脚复位标志
        PORRSTF     = 23,   //POR/PDR 复位标志
        SFTRSTF     = 24,   //通过 CPU 实现系统复位的复位标志
        IWDG1RSTF   = 26,   //独立看门狗复位标志
        WWDG1RSTF   = 28,   //窗口看门狗复位标志
        LPWRRSTF    = 30,   //因非法 D1 DStandby 或 CPU CStop 标志而复位
    };

    const char *status[] = 
    {
        [CPURSTF]   = "CPU reset flag",
        [D1RSTF]    = "D1 Domain power switch reset flag",
        [D2RSTF]    = "D2 Domain power switch reset flag",
        [BORRSTF]   = "BOR RST",
        [PINRSTF]   = "Pin reset flag",
        [PORRSTF]   = "POR/PDR RST",
        [SFTRSTF]   = "Reset flag for system reset through CPU",
        [IWDG1RSTF] = "Independent watchdog reset flag",
        [WWDG1RSTF] = "Window watchdog reset flag",
        [LPWRRSTF]  = "Reset due to invalid D1 DStandby or CPU CStop flag. Procedure",
    };

    rt_uint32_t rcc_rsr = READ_REG(RCC->RSR);
    rt_uint8_t i = 0;
    for(i = 0; i < LPWRRSTF; i++)
    {
        if((rcc_rsr << i) & 0x80000000)
        {
            break;
        }
    }
    rt_kprintf("rcc_rsr = 0x%08lx(%s)\r\n", rcc_rsr, status[31-i]);
}

/* re-implement tick interface for STM32 HAL */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    rt_hw_systick_init();

    /* Return function status */
    return HAL_OK;
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void _Error_Handler(char *s, int num)
{
    /* USER CODE BEGIN Error_Handler */
    /* User can add his own implementation to report the HAL error return state */
    LOG_E("Error_Handler at file:%s num:%d", s, num);
    while (1)
    {
    }
    /* USER CODE END Error_Handler */
}

/**
 * This function will delay for some us.
 *
 * @param us the delay time of us
 */
void rt_hw_us_delay(rt_uint32_t us)
{
    rt_uint64_t ticks;
    rt_uint32_t told, tnow, tcnt = 0;
    rt_uint32_t reload = SysTick->LOAD;

    ticks = us * (reload / (1000000 / RT_TICK_PER_SECOND));
    told = SysTick->VAL;
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

/**
 * This function will initial STM32 board.
 */
rt_weak void rt_hw_board_init(void)
{
#ifdef BSP_SCB_ENABLE_I_CACHE
    /* Enable I-Cache---------------------------------------------------------*/
    SCB_EnableICache();
#endif

#ifdef BSP_SCB_ENABLE_D_CACHE
    /* Enable D-Cache---------------------------------------------------------*/
    SCB_EnableDCache();
#endif

    /* HAL_Init() function is called at the beginning of the program */
    HAL_Init();

    /* System clock initialization */
    SystemClock_Config();

#if defined(RT_USING_HEAP)
    /* Heap initialization */
    rt_system_heap_init((void *)HEAP_BEGIN, (void *)HEAP_END);
#endif

    MX_GPIO_Init();

#ifdef RT_USING_PIN
    rt_hw_pin_init();
#endif

#ifdef RT_USING_SERIAL
    rt_hw_usart_init();
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_DEVICE)
    /* Set the shell console output device */
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);
#endif

#if defined(RT_USING_CONSOLE) && defined(RT_USING_NANO)
    extern void rt_hw_console_init(void);
    rt_hw_console_init();
#endif

    printf_Reset_Status();

#ifdef RT_USING_COMPONENTS_INIT
    /* Board underlying hardware initialization */
    rt_components_board_init();
#endif
/***********************调试模式下禁用独立看门狗IWDG**********************************/
    __HAL_DBGMCU_FREEZE_IWDG1();	  //调试模式下，冻结看门狗计数器时钟
    __HAL_DBGMCU_FREEZE_LPTIM1();
/*********************调试模式下使能独立看门狗IWDG**********************************/
//  __HAL_DBGMCU_UnFreeze_WWDG1();	  //调试模式下，使能看门狗计数器时钟
}
