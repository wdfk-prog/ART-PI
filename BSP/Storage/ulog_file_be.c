/**
 * @file ulog_file_be.c
 * @brief Product ULOG file backend routing.
 * @author HLY (1425075683@qq.com)
 * @copyright Copyright (c) 2022
 */
#include <rtconfig.h>

#if defined(BSP_USING_STORAGE_ULOG_FILE) && defined(RT_USING_ULOG) && defined(ULOG_BACKEND_USING_FILE)

#include "ulog_file_be.h"
#include <ulog_be.h>

struct log_file_entry
{
    const char *name;
    ulog_backend_t backend;
    struct ulog_file_be *file_be;
    const char *dir_path;
    rt_size_t max_num;
    rt_size_t max_size;
    rt_size_t buf_size;
};

enum log_file_id
{
    LOG_FILE_CONSOLE = 0,
    LOG_FILE_FLASH_SYS,
    LOG_FILE_FLASH_MOTION,
    LOG_FILE_SD_SYS,
};

#define FLASH_ROOT_PATH "/flash/log"
#define SD_ROOT_PATH    "/sdcard/log4"
#define FILE_SIZE       (512U * 1024U)
#define BUFFER_SIZE     512U

static struct ulog_backend flash_sys_log_backend;
static struct ulog_file_be flash_sys_log_file;
static struct ulog_backend flash_motion_log_backend;
static struct ulog_file_be flash_motion_log_file;
static struct ulog_backend sd_sys_log_backend;
static struct ulog_file_be sd_sys_log_file;

static struct log_file_entry log_files[] = {
    [LOG_FILE_CONSOLE] = {"console", RT_NULL, RT_NULL, RT_NULL, 0, 0, 0},
    [LOG_FILE_FLASH_SYS] = {"flash_sys", &flash_sys_log_backend, &flash_sys_log_file, FLASH_ROOT_PATH, 2, FILE_SIZE, BUFFER_SIZE},
    [LOG_FILE_FLASH_MOTION] = {"motion", &flash_motion_log_backend, &flash_motion_log_file, FLASH_ROOT_PATH, 8, FILE_SIZE, BUFFER_SIZE},
    [LOG_FILE_SD_SYS] = {"sd_sys", &sd_sys_log_backend, &sd_sys_log_file, SD_ROOT_PATH, 10, FILE_SIZE, BUFFER_SIZE},
};

static rt_bool_t sys_log_file_backend_filter(struct ulog_backend *backend,
                                              rt_uint32_t level,
                                              const char *tag,
                                              rt_bool_t is_raw,
                                              const char *log,
                                              rt_size_t len)
{
    RT_UNUSED(backend);
    RT_UNUSED(level);
    RT_UNUSED(is_raw);
    RT_UNUSED(log);
    RT_UNUSED(len);
    return (rt_strncmp(tag, MOTION_TAG, sizeof(MOTION_TAG)) == 0) ? RT_FALSE : RT_TRUE;
}

static rt_bool_t motion_log_file_backend_filter(struct ulog_backend *backend,
                                                 rt_uint32_t level,
                                                 const char *tag,
                                                 rt_bool_t is_raw,
                                                 const char *log,
                                                 rt_size_t len)
{
    RT_UNUSED(backend);
    RT_UNUSED(level);
    RT_UNUSED(is_raw);
    RT_UNUSED(log);
    RT_UNUSED(len);
    return (rt_strncmp(tag, MOTION_TAG, sizeof(MOTION_TAG)) == 0) ? RT_TRUE : RT_FALSE;
}

static rt_err_t init_file_backend(enum log_file_id id, ulog_backend_filter_t filter)
{
    struct log_file_entry *entry = &log_files[id];
    rt_err_t result;

    /* Repeated mount notifications must not allocate/register a backend twice. */
    if (ulog_backend_find(entry->name) != RT_NULL)
    {
        return RT_EOK;
    }

    entry->file_be->parent = *entry->backend;
    result = ulog_file_backend_init(entry->file_be,
                                    entry->name,
                                    entry->dir_path,
                                    entry->max_num,
                                    entry->max_size,
                                    entry->buf_size);
    if (result != RT_EOK)
    {
        return result;
    }

    ulog_file_backend_enable(entry->file_be);
    ulog_backend_set_filter(&entry->file_be->parent, filter);
    return RT_EOK;
}

void flash_sys_log_file_backend_init(void)
{
    if (init_file_backend(LOG_FILE_FLASH_SYS, sys_log_file_backend_filter) != RT_EOK)
    {
        rt_kprintf("ulog: init flash_sys backend failed\n");
    }
}

void flash_motion_log_file_backend_init(void)
{
    if (init_file_backend(LOG_FILE_FLASH_MOTION, motion_log_file_backend_filter) != RT_EOK)
    {
        rt_kprintf("ulog: init motion backend failed\n");
    }
}

void sdcard_sys_log_file_backend_init(void)
{
    if (init_file_backend(LOG_FILE_SD_SYS, sys_log_file_backend_filter) != RT_EOK)
    {
        rt_kprintf("ulog: init sd_sys backend failed\n");
    }
}

void sdcard_sys_log_file_backend_deinit(void)
{
    if (ulog_backend_find(log_files[LOG_FILE_SD_SYS].name) != RT_NULL)
    {
        /*
         * Deinit while the backend is still enabled so RT-Thread can flush its
         * buffered log data before closing the file and unregistering it.
         */
        ulog_file_backend_deinit(log_files[LOG_FILE_SD_SYS].file_be);
        ulog_file_backend_disable(log_files[LOG_FILE_SD_SYS].file_be);
    }
}

#ifdef RT_USING_MSH
static void print_separator(int len)
{
    while (len-- > 0)
    {
        rt_kprintf("-");
    }
}

static void cmd_log_file_backend(int argc, char **argv)
{
    rt_size_t i;

    if (argc < 2)
    {
        rt_kprintf("Usage:\n");
        rt_kprintf("  ulog_be_cmd list\n");
        rt_kprintf("  ulog_be_cmd deinit <name>\n");
        rt_kprintf("  ulog_be_cmd control <name> <enable|disable>\n");
        return;
    }

    if (rt_strcmp(argv[1], "list") == 0)
    {
        int maxlen = RT_NAME_MAX;
        rt_kprintf("%-*.*s init_state\n", maxlen, maxlen, "file_be");
        print_separator(maxlen);
        rt_kprintf(" ----------\n");
        for (i = 0; i < sizeof(log_files) / sizeof(log_files[0]); ++i)
        {
            rt_kprintf("%-*.*s  %s\n", maxlen, maxlen, log_files[i].name,
                       ulog_backend_find(log_files[i].name) != RT_NULL ? "init" : "deinit");
        }
        return;
    }

    if (rt_strcmp(argv[1], "deinit") == 0)
    {
        if (argc < 3)
        {
            rt_kprintf("Usage: ulog_be_cmd deinit <name>\n");
            return;
        }
        for (i = 1; i < sizeof(log_files) / sizeof(log_files[0]); ++i)
        {
            if (rt_strcmp(argv[2], log_files[i].name) == 0)
            {
                if (ulog_backend_find(log_files[i].name) == RT_NULL)
                {
                    rt_kprintf("The file backend %s is not initialized\n", argv[2]);
                    return;
                }
                /* Keep the backend enabled until deinit flushes buffered data. */
                ulog_file_backend_deinit(log_files[i].file_be);
                ulog_file_backend_disable(log_files[i].file_be);
                rt_kprintf("The file backend %s is deinit\n", argv[2]);
                return;
            }
        }
        rt_kprintf("File backend %s not found\n", argv[2]);
        return;
    }

    if (rt_strcmp(argv[1], "control") == 0)
    {
        if (argc < 4)
        {
            rt_kprintf("Usage: ulog_be_cmd control <name> <enable|disable>\n");
            return;
        }
        for (i = 1; i < sizeof(log_files) / sizeof(log_files[0]); ++i)
        {
            if (rt_strcmp(argv[2], log_files[i].name) == 0)
            {
                if (ulog_backend_find(log_files[i].name) == RT_NULL)
                {
                    rt_kprintf("The file backend %s is not initialized\n", argv[2]);
                    return;
                }
                if (rt_strcmp(argv[3], "enable") == 0)
                {
                    ulog_file_backend_enable(log_files[i].file_be);
                }
                else if (rt_strcmp(argv[3], "disable") == 0)
                {
                    ulog_file_backend_disable(log_files[i].file_be);
                }
                else
                {
                    rt_kprintf("Unknown state: %s\n", argv[3]);
                    return;
                }
                rt_kprintf("The file backend %s is %s\n", argv[2], argv[3]);
                return;
            }
        }
        rt_kprintf("File backend %s not found\n", argv[2]);
        return;
    }

    rt_kprintf("Unknown operation: %s\n", argv[1]);
}
MSH_CMD_EXPORT_ALIAS(cmd_log_file_backend, ulog_be_cmd, control ulog file backends);

#endif /* RT_USING_MSH */

#endif /* BSP_USING_STORAGE_ULOG_FILE && RT_USING_ULOG && ULOG_BACKEND_USING_FILE */
