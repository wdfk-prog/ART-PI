/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author          Notes
 * 2019-05-06     Zero-Free       first version
 * 2024-07-04     wdfk-prog       lptimer is register with hwtimer, only supports pm calls,the timer function is not supported
 */

#include <board.h>
#include <drv_lptim.h>
#include <rtdevice.h>
#include "drv_config.h"

//#define DRV_DEBUG
#define LOG_TAG             "drv.lptim"
#include <drv_log.h>

#ifdef BSP_USING_LPTIM

#define LPTIM_REG_MAX_VALUE (0xFFFF)

enum
{
#ifdef BSP_USING_LPTIM1
    LPTIM1_INDEX,
#endif
#ifdef BSP_USING_LPTIM2
    LPTIM2_INDEX,
#endif
#ifdef BSP_USING_LPTIM3
    LPTIM3_INDEX,
#endif
};

struct stm32_hw_lptimer
{
    rt_hwtimer_t        time_device;
    LPTIM_HandleTypeDef tim_handle;
    IRQn_Type           tim_irqn;
    char                *name;
};

static struct stm32_hw_lptimer stm32_hw_lptimer_obj[] =
{
#ifdef BSP_USING_LPTIM1
    LPTIM1_CONFIG,
#endif
#ifdef BSP_USING_LPTIM2
    LPTIM2_CONFIG,
#endif
#ifdef BSP_USING_LPTIM3
    LPTIM3_CONFIG,
#endif
};

static const struct rt_hwtimer_info _info = LPTIM_DEV_INFO_CONFIG;

static void timer_init(struct rt_hwtimer_device *timer, rt_uint32_t state)
{
    if(timer == RT_NULL)
    {
        LOG_E("init timer is NULL");
        return;
    }

    if (state)
    {
        struct stm32_hw_lptimer *tim_device = rt_container_of(timer, struct stm32_hw_lptimer, time_device);
        LPTIM_HandleTypeDef *tim = (LPTIM_HandleTypeDef *)timer->parent.user_data;

        if(tim_device == RT_NULL)
        {
            LOG_E("start tim_device is NULL");
            return;
        }
        if(tim == RT_NULL)
        {
            LOG_E("start %s LPTIM_Handle is NULL", tim_device->name);
            return;
        }

        RCC_OscInitTypeDef RCC_OscInitStruct = {0};
        RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {0};

        /* Enable LSI clock */
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
        RCC_OscInitStruct.LSIState = RCC_LSI_ON;
        RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
        HAL_RCC_OscConfig(&RCC_OscInitStruct);

        /* Select the LSI clock as LPTIM peripheral clock */
        RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
        RCC_PeriphCLKInitStruct.Lptim1ClockSelection = RCC_LPTIM1CLKSOURCE_LSI;
        HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);

        tim->Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
        tim->Init.Clock.Prescaler = LPTIM_PRESCALER_DIV32;
        tim->Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
        tim->Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
        tim->Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
        tim->Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;

        HAL_StatusTypeDef ret = HAL_LPTIM_Init(tim);
        if (ret != HAL_OK)
        {
            LOG_E("%s init failed %d", tim_device->name, ret);
        }
        else
        {
            NVIC_ClearPendingIRQ(LPTIM1_IRQn);
            NVIC_SetPriority(LPTIM1_IRQn, 0);
            NVIC_EnableIRQ(LPTIM1_IRQn);
            LOG_D("%s init success", tim_device->name);
        }
    }
}

static rt_err_t timer_start(rt_hwtimer_t *timer, rt_uint32_t t, rt_hwtimer_mode_t opmode)
{
    if(timer == RT_NULL)
    {
        LOG_E("start timer is NULL");
        return -RT_EINVAL;
    }

    struct stm32_hw_lptimer *tim_device = rt_container_of(timer, struct stm32_hw_lptimer, time_device);
    LPTIM_HandleTypeDef *tim = (LPTIM_HandleTypeDef *)timer->parent.user_data;

    if(tim_device == RT_NULL)
    {
        LOG_E("start tim_device is NULL");
        return -RT_EINVAL;
    }
    if(tim == RT_NULL)
    {
        LOG_E("start %s LPTIM_Handle is NULL", tim_device->name);
        return -RT_EINVAL;
    }

    HAL_StatusTypeDef ret = HAL_LPTIM_TimeOut_Start_IT(tim, LPTIM_REG_MAX_VALUE, t);
    if(ret != HAL_OK)
    {
        LOG_E("start %s failed %d", tim_device->name, ret);
        return -RT_ERROR;
    }
    else
    {
        LOG_D("start %s success", tim_device->name);
        return RT_EOK;
    }
}

static void timer_stop(rt_hwtimer_t *timer)
{
    if(timer == RT_NULL)
    {
        LOG_E("stop timer is NULL");
        return;
    }

    struct stm32_hw_lptimer *tim_device = rt_container_of(timer, struct stm32_hw_lptimer, time_device);
    LPTIM_HandleTypeDef *tim = (LPTIM_HandleTypeDef *)timer->parent.user_data;

    if(tim_device == RT_NULL)
    {
        LOG_E("stop tim_device is NULL");
        return;
    }
    if(tim == RT_NULL)
    {
        LOG_E("stop %s LPTIM_Handle is NULL", tim_device->name);
        return;
    }

    HAL_StatusTypeDef ret = HAL_LPTIM_TimeOut_Stop_IT(tim);
    if(ret != HAL_OK)
    {
        LOG_E("stop %s failed %d", tim_device->name, ret);
    }
    else
    {
        LOG_D("stop %s success", tim_device->name);
    }
}

static rt_uint32_t timer_get_freq(LPTIM_HandleTypeDef *tim)
{
    rt_uint32_t freq = 0;
    rt_uint32_t periph_clk = 0U;

    switch ((rt_uint32_t)&tim->Instance)
    {
        case (rt_uint32_t)LPTIM1:
            periph_clk = RCC_PERIPHCLK_LPTIM1;
            break;
        case (rt_uint32_t)LPTIM2:
            periph_clk = RCC_PERIPHCLK_LPTIM2;
            break;
        case (rt_uint32_t)LPTIM3:
            periph_clk = RCC_PERIPHCLK_LPTIM3;
            break;
        default:
            break;
    }

    if(periph_clk == 0U)
    {
        freq = 0;
    }
    else
    {
        freq = HAL_RCCEx_GetPeriphCLKFreq(periph_clk);
        switch (tim->Init.Clock.Prescaler)
        {
            case LPTIM_PRESCALER_DIV1:
            {
                break;
            }
            case LPTIM_PRESCALER_DIV2:
            {
                freq = freq / 2;
                break;
            }
            case LPTIM_PRESCALER_DIV4:
            {
                freq = freq / 4;
                break;
            }
            case LPTIM_PRESCALER_DIV8:
            {
                freq = freq / 8;
                break;
            }
            case LPTIM_PRESCALER_DIV16:
            {
                freq = freq / 16;
                break;
            }
            case LPTIM_PRESCALER_DIV32:
            {
                freq = freq / 32;
                break;
            }
            case LPTIM_PRESCALER_DIV64:
            {
                freq = freq / 64;
                break;
            }
            case LPTIM_PRESCALER_DIV128:
            {
                freq = freq / 128;
                break;
            }
            default:
            {
                freq = 0;
                break;
            }
        }
    }
    return freq;
}


static rt_uint32_t timer_counter_get(rt_hwtimer_t *timer)
{
    LPTIM_HandleTypeDef *tim = (LPTIM_HandleTypeDef *)timer->parent.user_data;
    return HAL_LPTIM_ReadCounter(tim);
}

static rt_err_t timer_ctrl(rt_hwtimer_t *timer, rt_uint32_t cmd, void *arg)
{
    if(timer == RT_NULL)
    {
        LOG_E("start timer is NULL");
        return -RT_EINVAL;
    }

    struct stm32_hw_lptimer *tim_device = rt_container_of(timer, struct stm32_hw_lptimer, time_device);
    LPTIM_HandleTypeDef *tim = (LPTIM_HandleTypeDef *)timer->parent.user_data;

    if(tim_device == RT_NULL)
    {
        LOG_E("start tim_device is NULL");
        return -RT_EINVAL;
    }
    if(tim == RT_NULL)
    {
        LOG_E("start %s LPTIM_Handle is NULL", tim_device->name);
        return -RT_EINVAL;
    }

    rt_err_t result = -RT_ERROR;
    switch (cmd)
    {
        case DRV_HW_LPTIMER_CTRL_GET_TICK_MAX:
        {
            *(rt_uint32_t *)arg = LPTIM_REG_MAX_VALUE;
            result = RT_EOK;
            break;
        }
        case DRV_HW_LPTIMER_CTRL_GET_FREQ:
        {
            *(rt_uint32_t *)arg = timer_get_freq(tim);
            break;
        }
        case DRV_HW_LPTIMER_CTRL_START:
        {
            timer_start(timer, *(rt_uint32_t *)arg, HWTIMER_MODE_ONESHOT);
            break;
        }
        case DRV_HW_LPTIMER_CTRL_GET_COUNT:
        {
            *(rt_uint32_t *)arg = timer_counter_get(timer);
            break;
        }
        default:
        {
            result = -RT_ENOSYS;
        }
        break;
    }

    return result;
}

#ifdef BSP_USING_LPTIM1
void LPTIM1_IRQHandler(void)
{
    HAL_LPTIM_IRQHandler(&stm32_hw_lptimer_obj[LPTIM1_INDEX].tim_handle);
}
#endif

#ifdef BSP_USING_LPTIM2
void LPTIM2_IRQHandler(void)
{
    HAL_LPTIM_IRQHandler(&stm32_hw_lptimer_obj[LPTIM2_INDEX].tim_handle);
}
#endif

#ifdef BSP_USING_LPTIM3
void LPTIM3_IRQHandler(void)
{
    HAL_LPTIM_IRQHandler(&stm32_hw_lptimer_obj[LPTIM3_INDEX].tim_handle);
}
#endif

static const struct rt_hwtimer_ops _ops =
{
    .init = timer_init,
    .start = timer_start,
    .stop = timer_stop,
    .count_get = timer_counter_get,
    .control = timer_ctrl,
};

/**
 * This function initialize the lptim
 */
static int stm32_hw_lptim_init(void)
{
    int i = 0;
    int result = RT_EOK;

    for (i = 0; i < sizeof(stm32_hw_lptimer_obj) / sizeof(stm32_hw_lptimer_obj[0]); i++)
    {
        stm32_hw_lptimer_obj[i].time_device.info = &_info;
        stm32_hw_lptimer_obj[i].time_device.ops  = &_ops;
        if (rt_device_hwtimer_register(&stm32_hw_lptimer_obj[i].time_device, stm32_hw_lptimer_obj[i].name, &stm32_hw_lptimer_obj[i].tim_handle) == RT_EOK)
        {
            LOG_D("%s register success", stm32_hw_lptimer_obj[i].name);
        }
        else
        {
            LOG_E("%s register failed", stm32_hw_lptimer_obj[i].name);
            result = -RT_ERROR;
        }
    }

    return result;
}

INIT_BOARD_EXPORT(stm32_hw_lptim_init);
#endif /* BSP_USING_LPTIM */