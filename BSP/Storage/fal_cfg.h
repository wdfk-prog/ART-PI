/*
 * File      : fal_cfg.h
 * This file is part of FAL (Flash Abstraction Layer) package
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-17     armink       the first version
 */

/*
 * Keep legacy partition start addresses stable because deployed devices may
 * already contain data at those offsets. CmBacktrace, when enabled, only
 * carves a small log partition from the tail of the existing filesystem area.
 * Enabling that option on media that already uses the full filesystem extent
 * still requires a filesystem migration or reformat before deployment.
 */
#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtconfig.h>
#include <board.h>

#define NOR_FLASH_DEV_NAME "norflash0"

#ifndef BSP_SPI_FLASH_SIZE_KB
#define BSP_SPI_FLASH_SIZE_KB (16 * 1024)
#endif /* BSP_SPI_FLASH_SIZE_KB */
#if (BSP_SPI_FLASH_SIZE_KB < 4104)
#error "BSP_SPI_FLASH_SIZE_KB must leave at least 8 KiB after the legacy 4 MiB prefix"
#endif /* BSP_SPI_FLASH_SIZE_KB < 4104 */
#define ART_PI_SPI_NOR_SIZE ((rt_size_t)BSP_SPI_FLASH_SIZE_KB * 1024U)

#define WIFI_IMAGE_OFFSET  (0U)
#define WIFI_IMAGE_SIZE    (512U * 1024U)
#define BT_IMAGE_OFFSET    (512U * 1024U)
#define BT_IMAGE_SIZE      (512U * 1024U)
#define DOWNLOAD_OFFSET    (1024U * 1024U)
#define DOWNLOAD_SIZE      (2U * 1024U * 1024U)
#define EASYFLASH_OFFSET   (3U * 1024U * 1024U)
#define EASYFLASH_SIZE     (1U * 1024U * 1024U)
#define FILESYSTEM_OFFSET  (4U * 1024U * 1024U)

extern struct fal_flash_dev nor_flash0;

#define FAL_FLASH_DEV_TABLE \
    {                       \
        &nor_flash0,        \
    }

#ifdef FAL_PART_HAS_TABLE_CFG

#ifdef CMB_USING_FAL_FLASH_LOG
#define CMB_LOG_SIZE (4U * 1024U)
#ifndef CMB_FAL_FLASH_LOG_PART
#define CMB_FAL_FLASH_LOG_PART "cmb_log"
#endif /* CMB_FAL_FLASH_LOG_PART */
#define FILESYSTEM_SIZE (ART_PI_SPI_NOR_SIZE - FILESYSTEM_OFFSET - CMB_LOG_SIZE)
#define FAL_PART_TABLE                                                                                                           \
    {                                                                                                                            \
        {FAL_PART_MAGIC_WORD, "wifi_image", NOR_FLASH_DEV_NAME, WIFI_IMAGE_OFFSET, WIFI_IMAGE_SIZE, 0},                         \
        {FAL_PART_MAGIC_WORD, "bt_image", NOR_FLASH_DEV_NAME, BT_IMAGE_OFFSET, BT_IMAGE_SIZE, 0},                               \
        {FAL_PART_MAGIC_WORD, "download", NOR_FLASH_DEV_NAME, DOWNLOAD_OFFSET, DOWNLOAD_SIZE, 0},                               \
        {FAL_PART_MAGIC_WORD, "easyflash", NOR_FLASH_DEV_NAME, EASYFLASH_OFFSET, EASYFLASH_SIZE, 0},                            \
        {FAL_PART_MAGIC_WORD, "filesystem", NOR_FLASH_DEV_NAME, FILESYSTEM_OFFSET, FILESYSTEM_SIZE, 0},                         \
        {FAL_PART_MAGIC_WORD, CMB_FAL_FLASH_LOG_PART, NOR_FLASH_DEV_NAME, ART_PI_SPI_NOR_SIZE - CMB_LOG_SIZE, CMB_LOG_SIZE, 0}, \
    }
#else
#define FILESYSTEM_SIZE (ART_PI_SPI_NOR_SIZE - FILESYSTEM_OFFSET)
#define FAL_PART_TABLE                                                                                   \
    {                                                                                                    \
        {FAL_PART_MAGIC_WORD, "wifi_image", NOR_FLASH_DEV_NAME, WIFI_IMAGE_OFFSET, WIFI_IMAGE_SIZE, 0}, \
        {FAL_PART_MAGIC_WORD, "bt_image", NOR_FLASH_DEV_NAME, BT_IMAGE_OFFSET, BT_IMAGE_SIZE, 0},       \
        {FAL_PART_MAGIC_WORD, "download", NOR_FLASH_DEV_NAME, DOWNLOAD_OFFSET, DOWNLOAD_SIZE, 0},       \
        {FAL_PART_MAGIC_WORD, "easyflash", NOR_FLASH_DEV_NAME, EASYFLASH_OFFSET, EASYFLASH_SIZE, 0},    \
        {FAL_PART_MAGIC_WORD, "filesystem", NOR_FLASH_DEV_NAME, FILESYSTEM_OFFSET, FILESYSTEM_SIZE, 0}, \
    }
#endif /* CMB_USING_FAL_FLASH_LOG */

#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */
