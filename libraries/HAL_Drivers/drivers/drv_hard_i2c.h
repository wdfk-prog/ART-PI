/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-02-17     Dyyt587      first version
 * 2024-06-23     wdfk-prog    Add timing Settings
 */

#ifndef __DRV_HARD_I2C_H__
#define __DRV_HARD_I2C_H__

#include "drv_config.h"
#include <rtthread.h>
#include "rtdevice.h"
#include <rthw.h>
#include <drv_common.h>
#include "drv_dma.h"
#include <ipc/completion.h>
#if defined(RT_USING_I2C) && defined(BSP_USING_I2C)

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

#define BSP_I2C_CTRL_SET_TIMING 0x40

struct stm32_i2c_config
{
    const char              *name;
    I2C_TypeDef             *Instance;
    rt_uint32_t             timing;
    rt_uint32_t             timeout;
    
    IRQn_Type               evirq_type;
    IRQn_Type               erirq_type;
#ifdef RT_I2C_USING_DMA
    struct dma_config       *dma_rx;
    struct dma_config       *dma_tx;
#endif /* RT_I2C_USING_DMA */
};

struct stm32_i2c
{
    I2C_HandleTypeDef           handle;
    struct stm32_i2c_config     *config;
    struct rt_i2c_bus_device    i2c_bus;
    rt_uint16_t                 i2c_dma_flag;
#if defined(RT_I2C_USING_INT) || defined(RT_I2C_USING_DMA)
    struct rt_completion        completion;
#endif /* defined(RT_I2C_USING_INT) || defined(RT_I2C_USING_DMA) */
#ifdef RT_I2C_USING_DMA
    struct
    {
        DMA_HandleTypeDef handle_rx;
        DMA_HandleTypeDef handle_tx;
    } dma;
#endif /* RT_I2C_USING_DMA */
};

#ifdef __cplusplus
}
#endif

#endif /* BSP_USING_I2C */

#endif /* __DRV_I2C_H__ */