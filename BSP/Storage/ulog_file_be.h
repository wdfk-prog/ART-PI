/**
 * @file ulog_file_be.h
 * @brief Product ULOG file backend interface.
 * @author HLY (1425075683@qq.com)
 * @copyright Copyright (c) 2022
 */
#ifndef __ULOG_FILE_BE_H__
#define __ULOG_FILE_BE_H__

#include <rtthread.h>
#include <rtdbg.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Tag reserved for motion-only log records. */
#define MOTION_TAG "MOVE"

/*
 * Keep product policy in Kconfig rather than a source-level constant. The
 * dedicated BSP switch defaults off to preserve the original ART-PI behavior,
 * while still requiring RT-Thread ULOG file-backend support when enabled.
 */
#if defined(BSP_USING_STORAGE_ULOG_FILE) && defined(RT_USING_ULOG) && defined(ULOG_BACKEND_USING_FILE)
#define OUT_FILE_ENABLE 1
#else
#define OUT_FILE_ENABLE 0
#endif /* BSP_USING_STORAGE_ULOG_FILE && RT_USING_ULOG && ULOG_BACKEND_USING_FILE */

/* Motion logging is independent of whether a file backend is enabled. */
#ifdef RT_USING_ULOG
#define LOG_MV(...) ulog_i(MOTION_TAG, __VA_ARGS__)
#else
#define LOG_MV(...) ((void)0)
#endif /* RT_USING_ULOG */

/** @brief Register the system-log file backend on the mounted SPI-NOR filesystem. */
void flash_sys_log_file_backend_init(void);

/** @brief Register the motion-log file backend on the mounted SPI-NOR filesystem. */
void flash_motion_log_file_backend_init(void);

/** @brief Register the system-log file backend on the mounted SD-card filesystem. */
void sdcard_sys_log_file_backend_init(void);

/** @brief Close and disable the SD-card log backend before the filesystem is unmounted. */
void sdcard_sys_log_file_backend_deinit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ULOG_FILE_BE_H__ */
