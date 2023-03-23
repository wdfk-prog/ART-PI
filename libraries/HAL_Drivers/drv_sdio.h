/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-05-23     liuduanfei   first version
 * 2020-08-25     wanghaijing  add sdmmmc2
 */

#ifndef __DRV_SDIO_H__
#define __DRV_SDIO_H__

#include <rtthread.h>
#include "rtdevice.h"
#include <rthw.h>
#include <drv_common.h>
#include <string.h>
#include <drivers/mmcsd_core.h>
#include <drivers/sdio.h>

#define SDCARD_INSTANCE_TYPE              SD_TypeDef

#if defined(SOC_SERIES_STM32F1) || defined(SOC_SERIES_STM32F2) || defined(SOC_SERIES_STM32F4)
#define SDCARD1_INSTANCE                   SDIO
#define SDCARD1_IRQn                       SDIO_IRQn
#define SDIO1_BASE_ADDRESS                (SDIO_BASE)
#define RCC_PERIPHCLK_SDCARD               RCC_PERIPHCLK_SDIO
#elif defined(SOC_SERIES_STM32L4) || defined(SOC_SERIES_STM32F7) || defined(SOC_SERIES_STM32H7)
#define SDCARD1_INSTANCE                   SDMMC1
#define SDCARD1_IRQn                       SDMMC1_IRQn
#define SDIO1_BASE_ADDRESS                (SDMMC1_BASE)
#define RCC_PERIPHCLK_SDCARD               RCC_PERIPHCLK_SDMMC
#endif /*  defined(SOC_SERIES_STM32F1) || defined(SOC_SERIES_STM32F4) */

#ifdef BSP_USING_SDIO2
#define SDCARD2_INSTANCE                   SDMMC2
#define SDCARD2_IRQn                       SDMMC2_IRQn
#define SDIO2_BASE_ADDRESS                (SDMMC2_BASE)
#endif /* BSP_USING_SDIO2 */

#define SDIO_BUFF_SIZE       4096
#define SDIO_ALIGN_LEN       32

#ifndef SDIO_CLOCK_FREQ
#define SDIO_CLOCK_FREQ      (200U * 1000 * 1000)
#endif

#ifndef SDIO_BUFF_SIZE
#define SDIO_BUFF_SIZE       (4096)
#endif

#ifndef SDIO_ALIGN_LEN
#define SDIO_ALIGN_LEN       (32)
#endif

#ifndef SDIO_MAX_FREQ
#define SDIO_MAX_FREQ        (25 * 1000 * 1000)
#endif

/* Frequencies used in the driver for clock divider calculation */
#define SD_INIT_FREQ                   400000U   /* Initalization phase : 400 kHz max */
#define SD_NORMAL_SPEED_FREQ           25000000U /* Normal speed phase : 25 MHz max */
#define SD_HIGH_SPEED_FREQ             50000000U /* High speed phase : 50 MHz max */

#define HW_SDIO_IT_CCRCFAIL                    (0x01U << 0)
#define HW_SDIO_IT_DCRCFAIL                    (0x01U << 1)
#define HW_SDIO_IT_CTIMEOUT                    (0x01U << 2)
#define HW_SDIO_IT_DTIMEOUT                    (0x01U << 3)
#define HW_SDIO_IT_TXUNDERR                    (0x01U << 4)
#define HW_SDIO_IT_RXOVERR                     (0x01U << 5)
#define HW_SDIO_IT_CMDREND                     (0x01U << 6)
#define HW_SDIO_IT_CMDSENT                     (0x01U << 7)
#define HW_SDIO_IT_DATAEND                     (0x01U << 8)
#if defined(SOC_SERIES_STM32F1) || defined(SOC_SERIES_STM32F2) || defined(SOC_SERIES_STM32F4)
#define HW_SDIO_IT_STBITERR                    (0x01U << 9)
#elif defined(SOC_SERIES_STM32L4) || defined(SOC_SERIES_STM32F7) || defined(SOC_SERIES_STM32H7)
#define HW_SDIO_IT_DHOLD                       (0x01U << 9)
#endif /*  defined(SOC_SERIES_STM32F1) || defined(SOC_SERIES_STM32F4) */
#define HW_SDIO_IT_DBCKEND                     (0x01U << 10)
#define HW_SDIO_IT_CMDACT                      (0x01U << 11)
#define HW_SDIO_IT_TXACT                       (0x01U << 12)
#define HW_SDIO_IT_RXACT                       (0x01U << 13)
#define HW_SDIO_IT_TXFIFOHE                    (0x01U << 14)
#define HW_SDIO_IT_RXFIFOHF                    (0x01U << 15)
#define HW_SDIO_IT_TXFIFOF                     (0x01U << 16)
#define HW_SDIO_IT_RXFIFOF                     (0x01U << 17)
#define HW_SDIO_IT_TXFIFOE                     (0x01U << 18)
#define HW_SDIO_IT_RXFIFOE                     (0x01U << 19)
#define HW_SDIO_IT_TXDAVL                      (0x01U << 20)
#define HW_SDIO_IT_RXDAVL                      (0x01U << 21)
#define HW_SDIO_IT_SDIOIT                      (0x01U << 22)

#define HW_SDIO_ERRORS \
    (HW_SDIO_IT_CCRCFAIL | HW_SDIO_IT_CTIMEOUT | \
     HW_SDIO_IT_DCRCFAIL | HW_SDIO_IT_DTIMEOUT | \
     HW_SDIO_IT_RXOVERR  | HW_SDIO_IT_TXUNDERR)

#define SDIO_MASKR_ALL \
    (SDMMC_MASK_CCRCFAILIE | SDMMC_MASK_DCRCFAILIE | SDMMC_MASK_CTIMEOUTIE | \
     SDMMC_MASK_TXUNDERRIE | SDMMC_MASK_RXOVERRIE | SDMMC_MASK_CMDRENDIE | \
     SDMMC_MASK_CMDSENTIE | SDMMC_MASK_DATAENDIE | SDMMC_MASK_ACKTIMEOUTIE)

#define HW_SDIO_DATATIMEOUT                    (0xFFFFFFFFU)

typedef rt_err_t (*dma_txconfig)(rt_uint32_t *src, rt_uint32_t *dst, int size);
typedef rt_err_t (*dma_rxconfig)(rt_uint32_t *src, rt_uint32_t *dst, int size);
typedef rt_uint32_t (*sdio_clk_get)(SD_HandleTypeDef *hsd);

struct stm32_sdio_des
{
    SD_HandleTypeDef hw_sdio;
    sdio_clk_get clk_get;
};

/* stm32 sdio dirver class */
struct stm32_sdio_class
{
    struct stm32_sdio_des *des;
    const struct stm32_sdio_config *cfg;
    struct rt_mmcsd_host host;
};

extern void stm32_mmcsd_change(void);

#endif /* __DRV_SDIO_H__ */
