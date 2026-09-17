/**
 * @file user_version.c
 * @brief ART-PI startup version report.
 * @author wdfk-prog (1425075683@qq.com)
 * @version 1.0
 * @date 2026-09-17
 * @copyright Copyright (c) 2026
 * @attention
 * @par Change Log:
 * Date       Version Author     Description
 * 2026-09-17 1.0     wdfk-prog first version
 */

#include <rtthread.h>
#include "main.h"

#define LOG_TAG "version"
#define LOG_LVL DBG_INFO
#include <ulog.h>

/**
 * @brief Print firmware, HAL, and compiler version information at startup.
 *
 * @return RT_EOK.
 */
static int user_version_init(void)
{
    uint32_t hal_version = HAL_GetHalVersion();

    LOG_I(" \\ | /");
    LOG_W("- HLY -    Version FULL V%s", VERSION);
    LOG_E(" / | \\     build %s %s", __DATE__, __TIME__);
    LOG_W("HAL version      = V%u.%u.%u",
          (unsigned int)((hal_version >> 24) & 0xFFU),
          (unsigned int)((hal_version >> 16) & 0xFFU),
          (unsigned int)((hal_version >> 8) & 0xFFU));

#if defined(__ARMCOMPILER_VERSION)
    LOG_W("ARM Compiler version = %lu", (unsigned long)__ARMCOMPILER_VERSION);
#elif defined(__ARMCC_VERSION)
    LOG_W("ARM Compiler version = %lu", (unsigned long)__ARMCC_VERSION);
#elif defined(__GNUC__)
    LOG_W("GCC version      = %u.%u.%u",
          (unsigned int)__GNUC__,
          (unsigned int)__GNUC_MINOR__,
          (unsigned int)__GNUC_PATCHLEVEL__);
#else
    LOG_W("Compiler version = unknown");
#endif /* defined(__ARMCOMPILER_VERSION) */

    return RT_EOK;
}
INIT_APP_EXPORT(user_version_init);
