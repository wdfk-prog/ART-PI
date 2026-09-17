#include <board.h>
#include <rtthread.h>
#include <rtdevice.h>

#ifndef RT_USING_TIMER_SOFT
    #error "Please enable soft timer feature!"
#endif

#define TIMER_APP_DEFAULT_TICK  (RT_TICK_PER_SECOND * 2)

#ifdef RT_USING_PM

static rt_timer_t timer1;

static void _timeout_entry(void *parameter)
{
    //rt_kprintf("current tick: %ld\n", rt_tick_get());
    static int i = 0;
    if(i++ == 5)
    {
        rt_pm_device_unregister(rt_device_find("uart1"));
        rt_pm_device_unregister(rt_device_find("uart4"));
        rt_pm_device_unregister(rt_device_find("rtc"));
    }
}

static int timer_app_init(void)
{
    rt_pm_device_register(rt_device_find("uart1"), RT_NULL);
    rt_pm_device_register(rt_device_find("uart4"), RT_NULL);
    rt_pm_device_register(rt_device_find("rtc"), RT_NULL);
    //释放模式
    rt_pm_release(PM_SLEEP_MODE_NONE);
    timer1 = rt_timer_create("timer_app",
                             _timeout_entry,
                             RT_NULL,
                             TIMER_APP_DEFAULT_TICK,
                             RT_TIMER_FLAG_PERIODIC | RT_TIMER_FLAG_SOFT_TIMER);
    if (timer1 != RT_NULL)
    {
        rt_timer_start(timer1);

        /* keep in timer mode */
        rt_pm_request(PM_SLEEP_MODE_DEEP);

        return 0;
    }
    else
    {
        return -1;
    }
}
INIT_APP_EXPORT(timer_app_init);

/**
 * @brief  Get the next system wake-up time
 * @note   When used by default, it goes into STANDBY mode and sleeps forever. tickless external rewriting is required
 * @param  mode: sleep mode
 * @retval timeout_tick
 */
rt_tick_t pm_timer_next_timeout_tick(rt_uint8_t mode)
{
    switch (mode)
    {
        case PM_SLEEP_MODE_LIGHT:
        case PM_SLEEP_MODE_DEEP:
        case PM_SLEEP_MODE_STANDBY:
            return rt_timer_next_timeout_tick();
    }

    return RT_TICK_MAX;
}

#endif /* RT_USING_PM */
