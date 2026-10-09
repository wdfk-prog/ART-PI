/**
 * @file reset_reason.c
 * @brief STM32H750 reset-cause capture and clear implementation.
 */
#include "reset_reason.h"

#include <stdbool.h>
#include "stm32h7xx_hal.h"

#include <rtthread.h>

/* Keep existing startup log tagging and filtering unchanged. */
#define LOG_TAG "version"
#define LOG_LVL DBG_INFO
#include <ulog.h>

/* The snapshot is per firmware image and does not survive a reset. */
static uint32_t s_reset_rsr;
static bool s_reset_captured;

void reset_reason_capture(void)
{
    if (s_reset_captured)
    {
        return;
    }

    s_reset_rsr = READ_REG(RCC->RSR);
    s_reset_captured = true;
}

void reset_reason_clear_flags(void)
{
    /* Preserve the first value even when clear is called before get. */
    reset_reason_capture();
    __HAL_RCC_CLEAR_RESET_FLAGS();
}

uint32_t reset_reason_get_rsr(void)
{
    reset_reason_capture();
    return s_reset_rsr;
}

/* STM32H750 RCC_RSR masks (rather than encoded HAL RCC_FLAG_* values). */
struct reset_flag_desc
{
    uint32_t mask;
    const char *description;
};

static const struct reset_flag_desc reset_flags[] = {
    {RCC_RSR_BORRSTF,   "Brown-out reset (BOR)"},
    {RCC_RSR_PINRSTF,   "NRST pin reset"},
    {RCC_RSR_PORRSTF,   "Power-on/PDR reset"},
    {RCC_RSR_SFTRSTF,   "Software system reset"},
    {RCC_RSR_IWDG1RSTF, "Independent watchdog 1 reset"},
    {RCC_RSR_WWDG1RSTF, "Window watchdog 1 reset"},
    {RCC_RSR_LPWRRSTF,  "Illegal low-power reset"},
    {RCC_RSR_CPURSTF,   "CPU reset"},
    {RCC_RSR_D1RSTF,   "D1 domain reset"},
    {RCC_RSR_D2RSTF,   "D2 domain reset"},
};

void reset_reason_log(void)
{
    const uint32_t rsr = reset_reason_get_rsr();
    rt_bool_t found = RT_FALSE;
    rt_size_t i;

    LOG_I("rcc_rsr = 0x%08lx", (unsigned long)rsr);
    for (i = 0; i < sizeof(reset_flags) / sizeof(reset_flags[0]); i++)
    {
        if ((rsr & reset_flags[i].mask) != 0U)
        {
            LOG_I("Reset flag: %s", reset_flags[i].description);
            found = RT_TRUE;
        }
    }

    if (found == RT_FALSE)
    {
        LOG_W("No RCC reset-cause flag captured");
    }
}
