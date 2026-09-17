#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

#define SOC_STM32H750XB

/* RT-Thread Kernel */

/* klibc options */

/* rt_vsnprintf options */

#define RT_KLIBC_USING_VSNPRINTF_LONGLONG
#define RT_KLIBC_USING_VSNPRINTF_STANDARD
#define RT_KLIBC_USING_VSNPRINTF_DECIMAL_SPECIFIERS
#define RT_KLIBC_USING_VSNPRINTF_EXPONENTIAL_SPECIFIERS
#define RT_KLIBC_USING_VSNPRINTF_WRITEBACK_SPECIFIER
#define RT_KLIBC_USING_VSNPRINTF_CHECK_NUL_IN_FORMAT_SPECIFIER
#define RT_KLIBC_USING_VSNPRINTF_INTEGER_BUFFER_SIZE 32
#define RT_KLIBC_USING_VSNPRINTF_DECIMAL_BUFFER_SIZE 32
#define RT_KLIBC_USING_VSNPRINTF_FLOAT_PRECISION 6
#define RT_KLIBC_USING_VSNPRINTF_MAX_INTEGRAL_DIGITS_FOR_DECIMAL 9
#define RT_KLIBC_USING_VSNPRINTF_LOG10_TAYLOR_TERMS 4
#define RT_KLIBC_USING_FULL_REPLACING_SPRINTF
#define RT_KLIBC_USING_FULL_REPLACING_SNPRINTF
#define RT_KLIBC_USING_FULL_REPLACING_PRINTF
#define RT_KLIBC_USING_FULL_REPLACING_VSPRINTF
#define RT_KLIBC_USING_FULL_REPLACING_VSNPRINTF
/* end of rt_vsnprintf options */

/* rt_vsscanf options */

/* end of rt_vsscanf options */

/* rt_memset options */

/* end of rt_memset options */

/* rt_memcpy options */

#define RT_KLIBC_USING_USER_MEMCPY
/* end of rt_memcpy options */

/* rt_memmove options */

/* end of rt_memmove options */

/* rt_memcmp options */

/* end of rt_memcmp options */

/* rt_strstr options */

/* end of rt_strstr options */

/* rt_strcasecmp options */

/* end of rt_strcasecmp options */

/* rt_strncpy options */

/* end of rt_strncpy options */

/* rt_strcpy options */

/* end of rt_strcpy options */

/* rt_strncmp options */

/* end of rt_strncmp options */

/* rt_strcmp options */

/* end of rt_strcmp options */

/* rt_strlen options */

/* end of rt_strlen options */

/* rt_strnlen options */

/* end of rt_strnlen options */
/* end of klibc options */
#define RT_NAME_MAX 16
#define RT_CPUS_NR 1
#define RT_ALIGN_SIZE 8
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 1000
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_HOOK_USING_FUNC_PTR
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 512
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO 4
#define RT_TIMER_THREAD_STACK_SIZE 512

/* kservice options */

#define RT_USING_TINY_FFS
/* end of kservice options */
#define RT_USING_DEBUG
#define RT_DEBUGING_ASSERT
#define RT_DEBUGING_COLOR
#define RT_DEBUGING_CONTEXT

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE
/* end of Inter-Thread communication */

/* Memory Management */

#define RT_USING_SMALL_MEM
#define RT_USING_MEMHEAP
#define RT_MEMHEAP_FAST_MODE
#define RT_USING_MEMHEAP_AS_HEAP
#define RT_USING_MEMHEAP_AUTO_BINDING
#define RT_USING_MEMTRACE
#define RT_USING_HEAP
/* end of Memory Management */
#define RT_USING_DEVICE
#define RT_USING_DEVICE_OPS
#define RT_USING_THREADSAFE_PRINTF
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 256
#define RT_CONSOLE_DEVICE_NAME "uart4"
#define RT_USING_CONSOLE_OUTPUT_CTL
#define RT_VER_NUM 0x50301
#define RT_BACKTRACE_LEVEL_MAX_NR 32
/* end of RT-Thread Kernel */
#define RT_USING_CACHE
#define RT_USING_HW_ATOMIC
#define ARCH_USING_HW_ATOMIC_8
#define ARCH_USING_HW_ATOMIC_16
#define RT_USING_CPU_FFS
#define ARCH_ARM
#define ARCH_ARM_CORTEX_M
#define ARCH_ARM_CORTEX_M7

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 2048
#define RT_MAIN_THREAD_PRIORITY 10
#define RT_USING_MSH
#define RT_USING_FINSH
#define FINSH_USING_MSH
#define FINSH_THREAD_NAME "tshell"
#define FINSH_THREAD_PRIORITY 20
#define FINSH_THREAD_STACK_SIZE 4096
#define FINSH_USING_HISTORY
#define FINSH_HISTORY_LINES 5
#define FINSH_USING_SYMTAB
#define FINSH_CMD_SIZE 80
#define MSH_USING_BUILT_IN_COMMANDS
#define FINSH_USING_DESCRIPTION
#define FINSH_ARG_MAX 10
#define FINSH_USING_OPTION_COMPLETION

/* DFS: device virtual file system */

#define RT_USING_DFS
#define DFS_USING_POSIX
#define DFS_USING_WORKDIR
#define DFS_FD_MAX 64
#define RT_USING_DFS_V1
#define DFS_FILESYSTEMS_MAX 5
#define DFS_FILESYSTEM_TYPES_MAX 5
#define RT_USING_DFS_ELMFAT

/* elm-chan's FatFs, Generic FAT Filesystem Module */

#define RT_DFS_ELM_CODE_PAGE 437
#define RT_DFS_ELM_WORD_ACCESS
#define RT_DFS_ELM_USE_LFN_3
#define RT_DFS_ELM_USE_LFN 3
#define RT_DFS_ELM_LFN_UNICODE_0
#define RT_DFS_ELM_LFN_UNICODE 0
#define RT_DFS_ELM_MAX_LFN 255
#define RT_DFS_ELM_DRIVES 2
#define RT_DFS_ELM_MAX_SECTOR_SIZE 512
#define RT_DFS_ELM_REENTRANT
#define RT_DFS_ELM_MUTEX_TIMEOUT 3000
/* end of elm-chan's FatFs, Generic FAT Filesystem Module */
#define RT_USING_DFS_DEVFS
#define RT_USING_DFS_ROMFS
/* end of DFS: device virtual file system */
#define RT_USING_FAL
#define FAL_USING_DEBUG
#define FAL_PART_HAS_TABLE_CFG
#define FAL_USING_SFUD_PORT
#define FAL_USING_NOR_FLASH_DEV_NAME "norflash0"
#define FAL_DEV_NAME_MAX 24
#define FAL_DEV_BLK_MAX 6

/* Device Drivers */

#define RT_USING_DEVICE_IPC
#define RT_UNAMED_PIPE_NUMBER 64
#define RT_USING_SERIAL
#define RT_USING_SERIAL_V2
#define RT_SERIAL_BUF_STRATEGY_OVERWRITE
#define RT_SERIAL_USING_DMA
#define RT_USING_MTD_NOR
#define RT_USING_RTC
#define RT_USING_SDIO
#define RT_SDIO_STACK_SIZE 512
#define RT_SDIO_THREAD_PRIORITY 15
#define RT_MMCSD_STACK_SIZE 2048
#define RT_MMCSD_THREAD_PRIORITY 22
#define RT_MMCSD_MAX_PARTITION 16
#define RT_USING_SPI
#define RT_USING_SPI_ISR
#define RT_USING_SFUD
#define RT_SFUD_USING_SFDP
#define RT_SFUD_USING_FLASH_INFO_TABLE
#define RT_SFUD_SPI_MAX_HZ 50000000
#define RT_USING_WDT
#define RT_USING_BLK

/* Partition Types */

#define RT_BLK_PARTITION_DFS
#define RT_BLK_PARTITION_EFI
/* end of Partition Types */
#define RT_USING_PIN
#define RT_USING_CHERRYUSB
#define RT_CHERRYUSB_DEVICE
#define RT_CHERRYUSB_DEVICE_SPEED_AUTO
#define RT_CHERRYUSB_DEVICE_DWC2_ST
#define RT_CHERRYUSB_DEVICE_CDC_ACM
#define RT_CHERRYUSB_DEVICE_MSC
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 512
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE 512
#define RT_CHERRYUSB_DEVICE_TEMPLATE_CDC_ACM_MSC
/* end of Device Drivers */

/* C/C++ and POSIX layer */

/* ISO-ANSI C layer */

/* Timezone and Daylight Saving Time */

/* end of Timezone and Daylight Saving Time */
/* end of ISO-ANSI C layer */

/* POSIX (Portable Operating System Interface) layer */

#define RT_USING_POSIX_FS

/* Interprocess Communication (IPC) */


/* Socket is in the 'Network' category */

/* end of Interprocess Communication (IPC) */
/* end of POSIX (Portable Operating System Interface) layer */
/* end of C/C++ and POSIX layer */

/* Network */

/* end of Network */

/* Memory protection */

/* end of Memory protection */

/* Utilities */

#define RT_USING_ULOG
#define ULOG_OUTPUT_LVL_D
#define ULOG_OUTPUT_LVL 7
#define ULOG_USING_ISR_LOG
#define ULOG_ASSERT_ENABLE
#define ULOG_LINE_BUF_SIZE 128
#define ULOG_USING_ASYNC_OUTPUT
#define ULOG_ASYNC_OUTPUT_BUF_SIZE 2048
#define ULOG_ASYNC_OUTPUT_BY_THREAD
#define ULOG_ASYNC_OUTPUT_THREAD_STACK 4096
#define ULOG_ASYNC_OUTPUT_THREAD_PRIORITY 30

/* log format */

#define ULOG_OUTPUT_FLOAT
#define ULOG_USING_COLOR
#define ULOG_OUTPUT_TIME
#define ULOG_TIME_USING_TIMESTAMP
#define ULOG_OUTPUT_LEVEL
#define ULOG_OUTPUT_TAG
/* end of log format */
#define ULOG_BACKEND_USING_CONSOLE
#define ULOG_BACKEND_USING_FILE
/* end of Utilities */

/* Using USB legacy version */

/* end of Using USB legacy version */
/* end of RT-Thread Components */

/* RT-Thread Utestcases */

/* end of RT-Thread Utestcases */
#define SOC_FAMILY_STM32
#define SOC_SERIES_STM32H7

/* ART-Pi BSP Configuration */

#define BOARD_STM32H750_ARTPI
#define SOC_STM32H750_ARTPI
#define BSP_SCB_ENABLE_I_CACHE
#define BSP_SCB_ENABLE_D_CACHE

/* Onboard peripherals */

#define BSP_USING_USB_TO_USART
#define BSP_USING_SPI_FLASH
#define BSP_SPI_FLASH_SIZE_KB 16384
#define BSP_USING_FS
#define BSP_USING_SDCARD_FS
#define BSP_USING_SPI_FLASH_FS
#define BSP_USING_STORAGE_AUTO_MKFS
#define BSP_USING_STORAGE_ULOG_FILE
#define ART_PI_CMB_FILE_BACKUP_STORAGE
#define CMB_FLASH_LOG_PART_WG 1
/* end of Onboard peripherals */

/* On-chip peripherals */

#define BSP_USING_GPIO
#define BSP_USING_UART
#define BSP_USING_UART4
#define BSP_UART4_RX_USING_DMA
#define BSP_UART4_RX_BUFSIZE 256
#define BSP_UART4_TX_BUFSIZE 0
#define BSP_UART4_DMA_PING_BUFSIZE 64
#define BSP_USING_SPI
#define BSP_USING_SPI1
#define BSP_SPI1_TX_USING_DMA
#define BSP_SPI1_RX_USING_DMA
#define BSP_USING_SDIO
#define BSP_USING_SDIO1
#define BSP_USING_SDRAM
#define BSP_USING_LCD
#define BSP_USING_LTDC
#define BSP_USING_ONCHIP_RTC
#define BSP_RTC_USING_LSE
#define BSP_USING_WDT
/* end of On-chip peripherals */
/* end of ART-Pi BSP Configuration */

/* Unified Libraries */

#define PKG_USING_STM32H7_CMSIS_DRIVER
#define PKG_USING_STM32H7_CMSIS_DRIVER_LATEST_VERSION
#define PKG_USING_STM32H7_HAL_DRIVER
#define PKG_USING_STM32H7_HAL_DRIVER_LATEST_VERSION
#define PKG_USING_CMSIS_CORE
#define PKG_USING_CMSIS_CORE_LATEST_VERSION
#define PKG_USING_CMBACKTRACE
#define PKG_CMBACKTRACE_PLATFORM_M7
#define PKG_CMBACKTRACE_DUMP_STACK
#define PKG_CMBACKTRACE_PRINT_CHINESE_UTF8
#define CMB_USING_FAL_FLASH_LOG
#define CMB_USING_FAL_BACKUP_LOG_TO_FILE
#define CMB_FAL_FLASH_LOG_PART "cmb_log"
#define CMB_LOG_FILE_PATH "/flash/log/cmb.log"
#define PKG_USING_CMBACKTRACE_LATEST_VERSION
#define PKG_CMBACKTRACE_VER_NUM 0x99999
#define PKG_USING_RT_MEMCPY_CM
#define PKG_USING_RT_MEMCPY_CM_LATEST_VERSION
#define PKG_USING_LITTLEFS
#define PKG_USING_LITTLEFS_LATEST_VERSION
#define LFS_READ_SIZE 256
#define LFS_PROG_SIZE 256
#define LFS_BLOCK_SIZE 4096
#define LFS_CACHE_SIZE 256
#define LFS_BLOCK_CYCLES 1000
#define LFS_THREADSAFE
#define LFS_LOOKAHEAD_MAX 128
#define RT_DEF_LFS_DRIVERS 1
#define PKG_USING_SYSWATCH
#define SYSWATCH_EXCEPT_RESOLVE_MODE_1
#define SYSWATCH_EXCEPT_RESOLVE_MODE 1
#define SYSWATCH_EXCEPT_TIMEOUT 60
#define SYSWATCH_EXCEPT_CONFIRM_TMO 15
#define SYSWATCH_EXCEPT_RESUME_DLY 15
#define SYSWATCH_THREAD_PRIO 0
#define SYSWATCH_THREAD_STK_SIZE 1024
#define SYSWATCH_THREAD_NAME "syswatch"
#define SYSWATCH_WDT_NAME "wdt"
#define SYSWATCH_WDT_TIMEOUT 5
#define PKG_USING_SYSWATCH_LATEST_VERSION
#define PKG_USING_MFBD
#define PKG_MFBD_BTN_CODE_SIZE_2BYTE
#define PKG_MFBD_BTN_CODE_SIZE 2
#define PKG_MFBD_BTN_COUNT_SIZE_2BYTE
#define PKG_MFBD_BTN_COUNT_SIZE 2
#define PKG_MFBD_BTN_INDEX_SIZE_4BYTE
#define PKG_MFBD_BTN_INDEX_SIZE 4
#define PKG_MFBD_USE_MULTIFUCNTION_BUTTON
#define PKG_MFBD_PARAMS_SAME_IN_GROUP
#define PKG_MFBD_MULTICLICK_STATE_AUTO_RESET
#define PKG_USING_MFBD_LATEST_VERSION
#define PKG_MFBD_VER_NUM 0x999999
/* end of Unified Libraries */

/* User Application Config */

/* User Drivers */

/* end of User Drivers */
/* end of User Application Config */

#endif
