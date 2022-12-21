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
/**
  * @brief  None.
  * @param  None.
  * @retval None.
  * @note   None.
*/
void get_nvic_irq()
{
  rt_kprintf("ldx E P A Priotity\n");
  for (rt_uint8_t i = 0; i < 255; i++)
  {
      if(NVIC_GetEnableIRQ(i))
      {
          rt_kprintf("%3d 1",i);
          NVIC_GetPendingIRQ(i) ? rt_kprintf(" 1") : rt_kprintf(" 0");
          NVIC_GetActive(i) ? rt_kprintf(" 1") : rt_kprintf(" 0");
          rt_kprintf(" %02d\n",NVIC_GetPriority(i));
      }
  }
}
MSH_CMD_EXPORT(get_nvic_irq, get all enable NVIC_IRQ);
