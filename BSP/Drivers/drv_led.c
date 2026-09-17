/**
  ******************************************************************************
  * @file    led.c
  * @brief   led
  ******************************************************************************
  * @attention https://blog.csdn.net/weixin_35698805/article/details/112614144
  * @author HLY
  对LED.C函数进行改造，实现模式枚举，增加辨识度。添加获取当前LED模式函数       2022.04.18
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "drv_led.h"
/* Private includes ----------------------------------------------------------*/
#include "gpio.h"
/* Private define ------------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
iLed_t *head_led = NULL;
uint32_t _led_ticks = 0;
/* Private function prototypes -----------------------------------------------*/
/**
 * @brief  led_init：LED初始化
 * @param  iLed_t *handle：   LED结构体指针
 * @param  void (*on)(void)： LED点亮函数指针
 * @param  void (*off)(void)：LED点暗函数指针
 * @retval None.
 * @note   主要是注册led
 */
void led_init(iLed_t *handle, void (*on)(void), void (*off)(void))
{
    rt_memset(handle, 0, sizeof(struct iLed_t)); // 初始化结构体内部清零

    handle->on = on;
    handle->off = off;
    handle->modeIndex = 0;
    handle->modeLen = 0;
    handle->tickCnt = 0;
    handle->state = 0;

    handle->off();
}
/**
  * @brief  led_reg_call_back：LED执行完成回调函数
  * @param  iLed_t *handle：   LED结构体指针
  * @param  void (*overCallBack)(void)：回调函数指针
  * @retval None.
  * @note   执行完成回调，可以不使用，
            当一个led灯执行完成之后会调用此函数
*/
void led_reg_call_back(iLed_t *handle, void (*overCallBack)(iLed_t *hled))
{
    handle->overCallBack = overCallBack;
}
/**
 * @brief  led_start：启动led灯
 * @param  iLed_t *handle：   LED结构体指针
 * @retval
 * @note   None.
 */
int8_t led_start(iLed_t *handle)
{
    struct iLed_t *target = head_led;

    while (target)
    {
        if (target == handle)
            return -1;
        target = target->next;
    }
    handle->next = head_led;
    head_led = handle;
    return 0;
}
/**
 * @brief  led_creat：创建led
 * @param  None.
 * @retval None.
 * @note   注册并启动
 */
int8_t led_creat(iLed_t *handle, void (*on)(void), void (*off)(void))
{
    led_init(handle, on, off);
    return led_start(handle);
}
/**
 * @brief  设置led模式
 * @param  int16_t times：正数：执行次数。负值无限执行
 * @retval None.
 * @note   None.
 */
void led_set_mode(iLed_t *handle, uint16_t *mode, uint16_t modeLen, int16_t times)
{
    struct iLed_t *target = head_led;

    while (target)
    {
        if (target == handle)
        {
            target->mode = mode;
            target->times = times;
            target->modeLen = modeLen / sizeof(mode[0]);
            break;
        }
        else
        {
            target = target->next;
        }
    }
}

// led灯停止
void led_stop(iLed_t *handle)
{
    struct iLed_t **curr;
    for (curr = &head_led; *curr;)
    {
        struct iLed_t *entry = *curr;
        if (entry == handle)
        {
            *curr = entry->next;
        }
        else
            curr = &entry->next;
    }
}

// led开关
void led_switch(iLed_t *handle, char state)
{
    switch (state)
    {
    case 1:
        handle->on();
        handle->state = state;
        break;
    case 0:
        handle->off();
        handle->state = state;
        break;
    default:
        break;
    }
}

// 触发反转
void led_toggle(iLed_t *handle)
{
    handle->state = !handle->state;
    led_switch(handle, handle->state);
}

// led主要的控制逻辑
void led_handle(iLed_t *handle)
{
    if ((handle->times != 0) && (handle->modeLen > 0))
    {
        handle->tickCnt++;
        if ((handle->tickCnt * LED_TICK_TIME) <= (handle->mode[handle->modeIndex])) // 执行次数小于等于周期闪烁次数
        {
            led_switch(handle, !(handle->modeIndex % 2));
        }
        else // 重新赋值，进入下一周期
        {
            handle->tickCnt = 0;
            handle->modeIndex++;
            if ((handle->modeIndex) >= (handle->modeLen)) // 周期次数到了
            {
                handle->modeIndex = 0;
                if (handle->times > 0) // 执行次数
                {
                    handle->times--;
                    if (handle->times == 0) // 执行次数到了
                    {
                        if (handle->overCallBack != NULL)
                        {
                            handle->overCallBack(handle);
                        }
                    }
                }
            }
        }
    }
}
/*************************使用方法1***********************************/
// 可以放在系统循环中
void led_loop(void)
{
    struct iLed_t *target;
    if (_led_ticks >= LED_TICK_TIME)
    {
        for (target = head_led; target; target = target->next) // 通过链表查询
        {
            led_handle(target);
        }
        _led_ticks = 0;
    }
}
// 系统滴答定时器中1ms执行一次
void led_tick()
{
    _led_ticks++;
}
/*************************使用方法2***********************************/
// 定时调用该函数 调用周期为 LED_TICK_TIME
void led_tick_loop(void)
{
    struct iLed_t *target;

    for (target = head_led; target; target = target->next) // 通过链表查询
    {
        led_handle(target);
    }
}
