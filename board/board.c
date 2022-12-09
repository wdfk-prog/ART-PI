/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-07-29     RealThread   first version
 */

#include <rtthread.h>
#include <board.h>
#include <drv_common.h>

#define DBG_TAG "board"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);