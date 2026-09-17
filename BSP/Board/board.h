/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-07-29     RealThread   first version
 */
#ifndef __BOARD_H__
#define __BOARD_H__

#include <rtthread.h>
#include <stm32h7xx.h>
#include <drv_common.h>
#include <drv_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CHIP_FAMILY_STM32
#define CHIP_SERIES_STM32H7
#define CHIP_NAME_STM32H750XBHX

/* The application executes from the ART-Pi memory-mapped QSPI window. */
#define ROM_START              ((uint32_t)0x90000000)
#define ROM_SIZE               (16384)
#define ROM_END                ((uint32_t)(ROM_START + ROM_SIZE * 1024U))

#define RAM_START              ((uint32_t)0x24000000)
#define RAM_SIZE               (512)
#define RAM_END                ((uint32_t)(RAM_START + RAM_SIZE * 1024U))

#define BSP_CLOCK_SOURCE          "HSE"
#define BSP_CLOCK_SOURCE_FREQ_MHZ ((int32_t)0)
#define BSP_CLOCK_SYSTEM_FREQ_MHZ ((int32_t)480)

#define STM32_FLASH_START_ADRESS       ROM_START
#define STM32_FLASH_SIZE         ROM_SIZE
#define STM32_FLASH_END_ADDRESS  ROM_END

#define STM32_SRAM1_SIZE         RAM_SIZE
#define STM32_SRAM1_START        RAM_START
#define STM32_SRAM1_END          RAM_END

#if defined(__ARMCC_VERSION)
extern int Image$$RW_IRAM1$$ZI$$Limit;
#define HEAP_BEGIN (&Image$$RW_IRAM1$$ZI$$Limit)
#else
extern int __bss_end;
#define HEAP_BEGIN (&__bss_end)
#endif /* __ARMCC_VERSION */
#define HEAP_END STM32_SRAM1_END

void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BOARD_H__ */
