/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-07     wanghaijing  the first version
 */
#include <rtconfig.h>

#ifdef BSP_USING_SPI_FLASH

#include <rtthread.h>
#include <rtdevice.h>
#include <dev_spi_flash.h>
#include <drv_spi.h>
#include <drv_gpio.h>

#define DBG_TAG "spi.flash"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define SPI_BUS_NAME          "spi1"
#define SPI_DEVICE_NAME       "spi10"
#define SPI_FLASH_DEVICE_NAME "norflash0"
#define SPI_CS_PIN            GET_PIN(A, 4)

#ifndef RT_SFUD_SPI_MAX_HZ
#define RT_SFUD_SPI_MAX_HZ 50000000U
#endif /* RT_SFUD_SPI_MAX_HZ */

static int rt_flash_init(void)
{
    struct rt_spi_device *spi_dev;
    struct rt_spi_configuration cfg;
    rt_err_t result;

    extern rt_spi_flash_device_t rt_sfud_flash_probe(const char *spi_flash_dev_name,
                                                      const char *spi_dev_name);

    result = rt_hw_spi_device_attach(SPI_BUS_NAME, SPI_DEVICE_NAME, SPI_CS_PIN);
    if (result != RT_EOK)
    {
        LOG_E("attach %s to %s failed: %d", SPI_DEVICE_NAME, SPI_BUS_NAME, result);
        return result;
    }

    if (rt_sfud_flash_probe(SPI_FLASH_DEVICE_NAME, SPI_DEVICE_NAME) == RT_NULL)
    {
        LOG_E("probe %s on %s failed", SPI_FLASH_DEVICE_NAME, SPI_DEVICE_NAME);
        return -RT_ERROR;
    }

    spi_dev = (struct rt_spi_device *)rt_device_find(SPI_DEVICE_NAME);
    if (spi_dev == RT_NULL)
    {
        LOG_E("find %s failed", SPI_DEVICE_NAME);
        return -RT_ERROR;
    }

    cfg.data_width = 8;
    cfg.mode = RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.max_hz = RT_SFUD_SPI_MAX_HZ;
    result = rt_spi_configure(spi_dev, &cfg);
    if (result != RT_EOK)
    {
        LOG_E("configure %s failed: %d", SPI_DEVICE_NAME, result);
        return result;
    }

    LOG_I("%s ready on %s, max_hz=%u", SPI_FLASH_DEVICE_NAME, SPI_BUS_NAME, cfg.max_hz);
    return RT_EOK;
}
INIT_ENV_EXPORT(rt_flash_init);

#endif /* BSP_USING_SPI_FLASH */
