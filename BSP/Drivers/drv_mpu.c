/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-14     whj4674672   first version
 */
#include <rtthread.h>
#include "stm32h7xx.h"
#include "board.h"

/*属性为Normal，cache的属性为 write-back，即仅更新cache，
 *在合适的时候(由cache策略决定或者软件强制更新)将数据更新到相应的SRAM空间
 *特别注意：如果要数据立即更新，写之后要SCB_CleanDCache，读数据时要SCB_InvalidateDCache
 */
#define MPU_Normal_WB        0x00


/*属性为Normal，cache的属性为 write-back，即仅更新cache，
 *在合适的时候(由cache策略决定或者软件强制更新)将数据更新到相应的SRAM空间
 *特别注意：如果要数据立即更新，写之后要SCB_CleanDCache，读数据时要SCB_InvalidateDCache
 */
#define MPU_Normal_WBWARA    0x01  //外部和内部写入无写入分配


/*属性为 normal，cache的属性为 Write-through，即更新cache的同时，
 *将数据同时写入相应的物理地址空间
 *特别注意：如果要数据立即更新，可以直接往内存写数据，但读数据时要SCB_InvalidateDCache
 */
#define MPU_Normal_WT         0x02


/*属性为 normal，禁用共享,禁用缓存
 */
#define MPU_Normal_NonCache   0x03


/*属性为 Device，共享设置无效，禁用共享,禁用缓存
 */
#define MPU_Device_NonCache   0x04

/**
  * @brief  配置MPU区域属性和大小寄存器值
    * @param  Region            MPU保护区域，取值范围（0—7）
    * @param  AccessPermission  数据访问权限，取值范围（MPU_NO_ACCESS—MPU_PRIV_RO_URO）
    * @param  TypeExtField      用于配置 Cache 策略，可以取值（0，1，2），一般只用0和1
    *
  * @param  Address             MPU保护区域基地址，特别注意配置的Address需要被Size整除
  * @param  Size                MPU保护区域大小,可以取值（MPU_1KB，MPU_4KB ...MPU_512MB）
  * @param  IsShareable       保护的存储空间是否可以共享，1=允许共享，0=禁止共享。
  * @param  IsCacheable         保护的存储空间是否可以缓存，1=允许缓存，0=禁止缓存。
  * @param  IsBufferable        使能Cache之后，策略是write-through还是write-back(bufferable)
    *                           1=允许缓冲，即回写（write-back），0=禁止缓冲，即直写（write-through）。
  * @retval None
  */
static void BSP_MPU_ConfigRegion(uint8_t    Number,
                                 uint8_t    TypeExtField,
                                 uint32_t   Address,
                                 uint32_t   Size,
                                 uint8_t    IsBufferable,
                                 uint8_t    IsCacheable)
{
    MPU_Region_InitTypeDef MPU_InitStruct;

    /* 禁用MPU */
    HAL_MPU_Disable();

    /* 配置MPU属性*/
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    MPU_InitStruct.BaseAddress = Address;                      //区域基地址。
    MPU_InitStruct.Size = Size;                                //要配置的区域的区域大小。
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;  //数据访问权限允许您配置用户和特权模式的读/写访问权限。
    MPU_InitStruct.IsBufferable = IsBufferable;                //区域是可缓冲的，即使用回写缓存。 可缓存但不可缓冲的区域使用直写策略。WB
    MPU_InitStruct.IsCacheable = IsCacheable;                  //区域是否可缓存的，即其值是否可以保存在缓存中。//M7 内核只要开启了 Cache，read allocate 就是开启的
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;     //区域是否可以在多个总线主控器之间共享。H7 的应用笔记对齐的描述是开启共享基本等同于关闭 Cache
    MPU_InitStruct.Number = Number;                            //区域号。
    MPU_InitStruct.TypeExtField = TypeExtField;                //键入扩展字段，允许您配置内存访问类型。
    MPU_InitStruct.SubRegionDisable = 0x00;                    //子区域禁用字段。
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;//指令访问禁用位，0=允许指令访问，1=禁止指令访问

    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    /* 启用MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT); //表示禁止了背景区，访问任何未使能 MPU 的区域均会造成内存异常 MemFault
}

void cpu_mpu_config(uint8_t Region, uint8_t Mode, uint32_t Address, uint32_t Size)
{
    switch (Mode)
    {
    case MPU_Normal_WB:
        /*write back,no write allocate */
        /* 设置内存为Normal类型,禁用共享, 回写模式不带写入读取分配*/
        BSP_MPU_ConfigRegion(Region, MPU_TEX_LEVEL0, Address, Size, MPU_ACCESS_BUFFERABLE, MPU_ACCESS_CACHEABLE);
        break;

    case MPU_Normal_WBWARA:
        /*write back,write and read allocate */
        /* 设置内存为Normal类型,禁用共享, 回写模式带写入读取分配*/
        BSP_MPU_ConfigRegion(Region, MPU_TEX_LEVEL1, Address, Size, MPU_ACCESS_BUFFERABLE, MPU_ACCESS_CACHEABLE);
        break;

    case MPU_Normal_WT:
        /*write through,no write allocate */
        /* 设置内存为Normal类型,禁用共享, 直写模式*/
        BSP_MPU_ConfigRegion(Region, MPU_TEX_LEVEL0, Address, Size, MPU_ACCESS_NOT_BUFFERABLE, MPU_ACCESS_CACHEABLE);
        break;

    case MPU_Normal_NonCache:
        /* 设置内存为Normal类型,禁用共享，禁用缓存模式*/
        BSP_MPU_ConfigRegion(Region, MPU_TEX_LEVEL1, Address, Size, MPU_ACCESS_NOT_BUFFERABLE, MPU_ACCESS_NOT_CACHEABLE);
        break;
    }

}

int mpu_init(void)
{
    cpu_mpu_config(MPU_REGION_NUMBER0, MPU_Normal_WT,       0x24000000, MPU_REGION_SIZE_512KB);
#ifdef BSP_USING_SDRAM
    cpu_mpu_config(MPU_REGION_NUMBER1, MPU_Normal_WT,       0xC0000000, MPU_REGION_SIZE_32MB);
#endif

#ifdef BSP_USING_ETH
    cpu_mpu_config(MPU_REGION_NUMBER2, MPU_Normal_WT,       0x30040000, MPU_REGION_SIZE_256B);
    cpu_mpu_config(MPU_REGION_NUMBER3, MPU_Normal_WT,       0x30044000, MPU_REGION_SIZE_16KB);
#endif
    cpu_mpu_config(MPU_REGION_NUMBER4, MPU_Normal_WT,       0x90000000, MPU_REGION_SIZE_8MB);
    cpu_mpu_config(MPU_REGION_NUMBER5, MPU_Normal_NonCache, 0x24070000, MPU_REGION_SIZE_64KB);
    return RT_EOK;
}
INIT_BOARD_EXPORT(mpu_init);
