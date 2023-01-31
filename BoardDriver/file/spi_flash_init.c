/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-07     wanghaijing  the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <spi_flash.h>
#include <drv_spi.h>

#define SPI_DEVICE_NAME     "spi10"

static int rt_flash_init(void)
{
    extern rt_spi_flash_device_t rt_sfud_flash_probe(const char *spi_flash_dev_name, const char *spi_dev_name);

    __HAL_RCC_GPIOA_CLK_ENABLE();
    rt_hw_spi_device_attach("spi1", "spi10", GET_PIN(A, 4));
    /* initialize SPI Flash device */
    if (RT_NULL == rt_sfud_flash_probe("norflash0", SPI_DEVICE_NAME))
    {
        return -RT_ERROR;
    }
    
    struct rt_spi_device *spi_dev;
 /* 查找 spi 设备获取设备句柄 */
    spi_dev = (struct rt_spi_device *)rt_device_find(SPI_DEVICE_NAME);
    if (!spi_dev)
    {
        rt_kprintf("spi sample run failed! can't find %s device!\n", SPI_DEVICE_NAME);
    }
    else
    {
       struct rt_spi_configuration cfg;
      cfg.data_width = 8;
      cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB;
      cfg.max_hz = 100 * 1000 *1000;//100MHZ
      rt_spi_configure(spi_dev,&cfg);
    }
    return RT_EOK;
}
INIT_ENV_EXPORT(rt_flash_init);