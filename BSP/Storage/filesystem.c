/**
 * @file filesystem.c
 * @brief ART-Pi ROMFS, SD-card and optional SPI-NOR filesystem integration.
 * @author HLY (1425075683@qq.com)
 * @copyright Copyright (c) 2022
 */
#include <rtconfig.h>

#ifdef BSP_USING_FS

#include <rtthread.h>
#include <dfs_fs.h>
#include <dfs_file.h>
#include <dfs_romfs.h>
#include <unistd.h>

#include "ulog_file_be.h"

#ifdef BSP_USING_SDCARD_FS
#include <board.h>
#include <drv_sdmmc.h>
#endif /* BSP_USING_SDCARD_FS */

#ifdef BSP_USING_SPI_FLASH_FS
#include <fal.h>
#endif /* BSP_USING_SPI_FLASH_FS */

#if defined(CMB_USING_FAL_BACKUP_LOG_TO_FILE) && !defined(BSP_USING_SPI_FLASH_FS)
#error "CmBacktrace file backup requires BSP_USING_SPI_FLASH_FS for CMB_LOG_FILE_PATH under /flash"
#endif /* CMB_USING_FAL_BACKUP_LOG_TO_FILE && !BSP_USING_SPI_FLASH_FS */

#define DBG_TAG "fs"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#if DFS_FILESYSTEMS_MAX < 4
#error "Please define DFS_FILESYSTEMS_MAX >= 4"
#endif /* DFS_FILESYSTEMS_MAX < 4 */
#if DFS_FILESYSTEM_TYPES_MAX < 4
#error "Please define DFS_FILESYSTEM_TYPES_MAX >= 4"
#endif /* DFS_FILESYSTEM_TYPES_MAX < 4 */

static const rt_uint8_t romfs_readme[] =
    "  \\ | /\r\n"
    " - HLY -    Version  V0.0.1\r\n"
    "  / | \\     \r\n";

static const struct romfs_dirent romfs_entries[] = {
    {ROMFS_DIRENT_DIR, "flash", RT_NULL, 0},
    {ROMFS_DIRENT_FILE, "readme.txt", (rt_uint8_t *)romfs_readme, sizeof(romfs_readme) - 1U},
    {ROMFS_DIRENT_DIR, "sdcard", RT_NULL, 0},
};

const struct romfs_dirent romfs_root = {
    ROMFS_DIRENT_DIR,
    "/",
    (rt_uint8_t *)romfs_entries,
    sizeof(romfs_entries) / sizeof(romfs_entries[0]),
};

#ifdef BSP_USING_SDCARD_FS
#define SD_CHECK_PIN GET_PIN(D, 5)

/*
 * A mount failure does not prove that media is unformatted. Automatic mkfs is
 * therefore an explicit board policy; disabling it turns the fallback into a
 * fail-closed mount error instead of erasing the affected storage device.
 */
static rt_err_t sdcard_mount(void)
{
    rt_device_t device = rt_device_find("sd0");

    if (device == RT_NULL)
    {
        mmcsd_wait_cd_changed(0);
        stm32_mmcsd_change();
        mmcsd_wait_cd_changed(RT_WAITING_FOREVER);
        device = rt_device_find("sd0");
    }

    if (device == RT_NULL)
    {
        LOG_E("find sd0 failed");
        return -RT_ERROR;
    }

    if (dfs_mount("sd0", "/sdcard", "elm", 0, RT_NULL) != RT_EOK)
    {
#ifdef BSP_USING_STORAGE_AUTO_MKFS
        LOG_W("mount /sdcard failed, formatting sd0 as elm");
        if (dfs_mkfs("elm", "sd0") != RT_EOK ||
            dfs_mount("sd0", "/sdcard", "elm", 0, RT_NULL) != RT_EOK)
        {
            LOG_E("mount /sdcard failed after format");
            return -RT_ERROR;
        }
#else
        LOG_E("mount /sdcard failed; automatic format is disabled");
        return -RT_ERROR;
#endif /* BSP_USING_STORAGE_AUTO_MKFS */
    }

    LOG_I("sd0 mounted at /sdcard");
#if (OUT_FILE_ENABLE == 1)
    sdcard_sys_log_file_backend_init();
#endif /* OUT_FILE_ENABLE == 1 */
    return RT_EOK;
}

static void sdcard_unmount(void)
{
#if (OUT_FILE_ENABLE == 1)
    /*
     * Stop and flush the ULOG file backend before the mount disappears.
     * This ordering prevents later log writes from targeting a detached SD
     * filesystem and leaves the backend ready for clean re-registration.
     */
    sdcard_sys_log_file_backend_deinit();
#endif /* OUT_FILE_ENABLE == 1 */
    rt_thread_mdelay(200);
    if (dfs_unmount("/sdcard") == RT_EOK)
    {
        LOG_I("/sdcard unmounted");
    }
    else
    {
        LOG_W("unmount /sdcard failed");
    }

    mmcsd_wait_cd_changed(0);
    stm32_mmcsd_change();
    mmcsd_wait_cd_changed(RT_WAITING_FOREVER);
}

static void sd_mount_thread(void *parameter)
{
    rt_base_t previous;

    RT_UNUSED(parameter);
    rt_thread_mdelay(200);
    previous = rt_pin_read(SD_CHECK_PIN);
    if (previous != 0)
    {
        sdcard_mount();
    }

    while (1)
    {
        rt_base_t current;
        rt_thread_mdelay(200);
        current = rt_pin_read(SD_CHECK_PIN);

        if (previous == 0 && current != 0)
        {
            sdcard_mount();
        }
        else if (previous != 0 && current == 0)
        {
            sdcard_unmount();
        }
        previous = current;
    }
}

#ifdef RT_USING_MSH
static void cmd_sdcard_speed_test(int argc, char **argv)
{
    enum { PATH_LEN = 128, BLOCK_SIZE = 1024, BLOCK_COUNT = 1024 };
    const char *dir_path = "/sdcard/test";
    char file_path[PATH_LEN];
    char buffer[BLOCK_SIZE];
    int file_fd;
    rt_uint32_t start;
    rt_uint32_t end;
    rt_uint32_t i;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    if (access(dir_path, F_OK) < 0)
    {
        mkdir(dir_path, 0);
    }

    rt_snprintf(file_path, sizeof(file_path), "%s/test.bin", dir_path);
    file_fd = open(file_path, O_CREAT | O_RDWR | O_TRUNC, 0);
    if (file_fd < 0)
    {
        rt_kprintf("open %s failed\n", file_path);
        return;
    }

    rt_memset(buffer, 0xA5, sizeof(buffer));
    start = rt_tick_get_millisecond();
    for (i = 0; i < BLOCK_COUNT; ++i)
    {
        if (write(file_fd, buffer, sizeof(buffer)) != (rt_ssize_t)sizeof(buffer))
        {
            rt_kprintf("write %s failed\n", file_path);
            close(file_fd);
            return;
        }
    }
    fsync(file_fd);
    end = rt_tick_get_millisecond();
    rt_kprintf("sd write 1 MiB: %lu ms\n", (unsigned long)(end - start));

    lseek(file_fd, 0, SEEK_SET);
    start = rt_tick_get_millisecond();
    for (i = 0; i < BLOCK_COUNT; ++i)
    {
        if (read(file_fd, buffer, sizeof(buffer)) != (rt_ssize_t)sizeof(buffer))
        {
            rt_kprintf("read %s failed\n", file_path);
            close(file_fd);
            return;
        }
    }
    end = rt_tick_get_millisecond();
    rt_kprintf("sd read 1 MiB: %lu ms\n", (unsigned long)(end - start));
    close(file_fd);
}
MSH_CMD_EXPORT_ALIAS(cmd_sdcard_speed_test, test_sdcard, test sdcard sequential speed);
#endif /* RT_USING_MSH */
#endif /* BSP_USING_SDCARD_FS */

#if defined(BSP_USING_SPI_FLASH_FS) && defined(CMB_USING_FAL_BACKUP_LOG_TO_FILE)
static rt_err_t cmb_log_parent_prepare(void)
{
    char path[sizeof(CMB_LOG_FILE_PATH)];
    rt_size_t i;
    rt_size_t path_len = sizeof(CMB_LOG_FILE_PATH) - 1U;

    if (path_len < 2U || CMB_LOG_FILE_PATH[0] != '/' || CMB_LOG_FILE_PATH[path_len - 1U] == '/')
    {
        LOG_E("invalid CMB_LOG_FILE_PATH: %s", CMB_LOG_FILE_PATH);
        return -RT_EINVAL;
    }

    rt_memcpy(path, CMB_LOG_FILE_PATH, sizeof(path));
    for (i = 1U; i < path_len; ++i)
    {
        if (path[i] != '/')
        {
            continue;
        }

        path[i] = '\0';
        if (access(path, F_OK) < 0 && mkdir(path, 0) < 0)
        {
            LOG_E("create CmBacktrace log directory %s failed", path);
            return -RT_ERROR;
        }
        path[i] = '/';
    }

    return RT_EOK;
}
#endif /* BSP_USING_SPI_FLASH_FS && CMB_USING_FAL_BACKUP_LOG_TO_FILE */

#ifdef BSP_USING_SPI_FLASH_FS
static rt_err_t spi_flash_filesystem_mount(void)
{
    struct rt_device *flash_dev = RT_NULL;
    const char *fs_name = RT_NULL;

    if (fal_init() < 0)
    {
        LOG_E("FAL init failed");
        return -RT_ERROR;
    }

#if defined(PKG_USING_LITTLEFS)
    fs_name = "lfs";
    flash_dev = fal_mtd_nor_device_create("filesystem");
#elif defined(RT_USING_DFS_ELMFAT)
    fs_name = "elm";
    flash_dev = fal_blk_device_create("filesystem");
#else
    LOG_E("no filesystem backend selected for SPI NOR");
    return -RT_ERROR;
#endif /* PKG_USING_LITTLEFS */

    if (flash_dev == RT_NULL)
    {
        LOG_E("create FAL device for filesystem partition failed");
        return -RT_ERROR;
    }

    if (dfs_mount(flash_dev->parent.name, "/flash", fs_name, 0, RT_NULL) != RT_EOK)
    {
#ifdef BSP_USING_STORAGE_AUTO_MKFS
        LOG_W("mount /flash failed, formatting %s as %s", flash_dev->parent.name, fs_name);
        if (dfs_mkfs(fs_name, flash_dev->parent.name) != RT_EOK ||
            dfs_mount(flash_dev->parent.name, "/flash", fs_name, 0, RT_NULL) != RT_EOK)
        {
            LOG_E("mount /flash failed after format");
            return -RT_ERROR;
        }
#else
        LOG_E("mount /flash failed; automatic format is disabled");
        return -RT_ERROR;
#endif /* BSP_USING_STORAGE_AUTO_MKFS */
    }

    LOG_I("%s mounted at /flash", flash_dev->parent.name);
#ifdef CMB_USING_FAL_BACKUP_LOG_TO_FILE
    /* CmBacktrace backs up at INIT_FS_EXPORT, immediately after APP init. */
    if (cmb_log_parent_prepare() != RT_EOK)
    {
        return -RT_ERROR;
    }
#endif /* CMB_USING_FAL_BACKUP_LOG_TO_FILE */
#if (OUT_FILE_ENABLE == 1)
    flash_sys_log_file_backend_init();
    flash_motion_log_file_backend_init();
#endif /* OUT_FILE_ENABLE == 1 */
    return RT_EOK;
}
#endif /* BSP_USING_SPI_FLASH_FS */

int mount_init(void)
{
    rt_err_t result = RT_EOK;

    /* Keep independent storage paths initializing, but preserve any failure. */
    if (dfs_mount(RT_NULL, "/", "rom", 0, (void *)&romfs_root) != RT_EOK)
    {
        LOG_E("mount ROMFS at / failed");
        result = -RT_ERROR;
    }

#ifdef BSP_USING_SPI_FLASH_FS
    if (spi_flash_filesystem_mount() != RT_EOK)
    {
        result = -RT_ERROR;
    }
#endif /* BSP_USING_SPI_FLASH_FS */

#ifdef BSP_USING_SDCARD_FS
    rt_thread_t tid;

    rt_pin_mode(SD_CHECK_PIN, PIN_MODE_INPUT_PULLUP);
    tid = rt_thread_create("sd_mount", sd_mount_thread, RT_NULL,
                           2048, RT_THREAD_PRIORITY_MAX - 2, 20);
    if (tid != RT_NULL)
    {
        if (rt_thread_startup(tid) != RT_EOK)
        {
            LOG_E("start sd_mount thread failed");
            rt_thread_delete(tid);
            result = -RT_ERROR;
        }
    }
    else
    {
        LOG_E("create sd_mount thread failed");
        result = -RT_ERROR;
    }
#endif /* BSP_USING_SDCARD_FS */

    return result;
}

/*
 * CmBacktrace uses INIT_FS_EXPORT for flash-log backup. Mount filesystems at
 * APP init first so /flash and CMB_LOG_FILE_PATH's parent directory exist
 * before that backup hook opens the destination file.
 */
INIT_APP_EXPORT(mount_init);

#endif /* BSP_USING_FS */
