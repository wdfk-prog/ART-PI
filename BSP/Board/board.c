/**
 * @file board.c
 * @brief ART-Pi board-level runtime helpers.
 * @author HLY (1425075683@qq.com)
 * @copyright Copyright (c) 2022
 */
#include "board.h"
#include "main.h"

#include <time.h>

#ifdef RT_USING_SERIAL
#ifdef RT_USING_SERIAL_V2
#include "drv_usart_v2.h"
#else
#include "drv_usart.h"
#endif /* RT_USING_SERIAL_V2 */
#endif /* RT_USING_SERIAL */

#define DBG_TAG "board"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define RTC_NAME "rtc"
#define PVD_ENABLE 0

#ifdef FINSH_IRQ_PRIORITY
static int set_finsh_irq(void)
{
    rt_device_t serial = rt_device_find(RT_CONSOLE_DEVICE_NAME);
    struct stm32_uart *uart;

    if (serial == RT_NULL)
    {
        LOG_E("find %s failed", RT_CONSOLE_DEVICE_NAME);
        return -RT_ERROR;
    }

    uart = rt_container_of(serial, struct stm32_uart, serial);
    RT_ASSERT(uart != RT_NULL);
    HAL_NVIC_SetPriority(uart->config->irq_type, FINSH_IRQ_PRIORITY, 0);
    return RT_EOK;
}
INIT_COMPONENT_EXPORT(set_finsh_irq);
#endif /* FINSH_IRQ_PRIORITY */

/*
 * ART-Pi links the application into the external QSPI XIP window. The vector
 * table must follow the image before interrupts are enabled by higher layers.
 */
static int vtor_config(void)
{
    SCB->VTOR = QSPI_BASE;
    return RT_EOK;
}
INIT_BOARD_EXPORT(vtor_config);

#ifdef RT_USING_RTC
static time_t rtc_get_build(void)
{
    struct tm build_time = {0};

    build_time.tm_year = YEAR - 1900;
    build_time.tm_mon = MONTH;
    build_time.tm_mday = DAY;
    build_time.tm_hour = HOUR;
    build_time.tm_min = MINUTE;
    build_time.tm_sec = SEC;

    return mktime(&build_time);
}

/* Kept callable for products that explicitly choose to sync RTC to build time. */
int rtc_update_init(void)
{
    rt_device_t device = rt_device_find(RTC_NAME);
    time_t build_time;

    if (device == RT_NULL)
    {
        LOG_E("find %s failed", RTC_NAME);
        return -RT_ERROR;
    }

    if (rt_device_open(device, 0) != RT_EOK)
    {
        LOG_E("open %s failed", RTC_NAME);
        return -RT_ERROR;
    }

    build_time = rtc_get_build();
    set_timestamp(build_time);
    return RT_EOK;
}
#endif /* RT_USING_RTC */

#if (PVD_ENABLE == 1)
#include <ulog.h>

static struct rt_completion pvd_completion;

static void pvd_thread_entry(void *parameter)
{
    RT_UNUSED(parameter);

    while (1)
    {
        rt_completion_wait(&pvd_completion, RT_WAITING_FOREVER);
        ulog_flush();
        LOG_W("PVD event: ULOG buffer flushed");
    }
}

static int pvd_init(void)
{
    PWR_PVDTypeDef config;
    rt_thread_t tid;

    HAL_NVIC_SetPriority(PVD_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(PVD_IRQn);

    config.PVDLevel = PWR_PVDLEVEL_6;
    config.Mode = PWR_PVD_MODE_IT_RISING;
    HAL_PWR_ConfigPVD(&config);
    HAL_PWR_EnablePVD();

    rt_completion_init(&pvd_completion);
    tid = rt_thread_create("PVD", pvd_thread_entry, RT_NULL, 512, 1, 20);
    if (tid == RT_NULL)
    {
        LOG_E("PVD thread start failed");
        return -RT_ERROR;
    }

    rt_thread_startup(tid);
    return RT_EOK;
}
INIT_APP_EXPORT(pvd_init);

void HAL_PWR_PVDCallback(void)
{
    if (__HAL_PWR_GET_FLAG(PWR_FLAG_PVDO))
    {
        rt_completion_done(&pvd_completion);
        LOG_E("voltage below PVD threshold detected");
    }
}
#endif /* PVD_ENABLE */
