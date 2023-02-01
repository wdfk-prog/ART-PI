/**
  ******************************************************************************
  * @file    drv_led.H
  * @brief   drv_led
  ******************************************************************************
  * @attention https://blog.csdn.net/weixin_35698805/article/details/112614144
  * @author
  对LED.C函数进行改造，实现模式枚举，增加辨识度。添加获取当前LED模式函数       2022.04.18
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRV_LED_H
#define __DRV_LED_H
/*
使用方法：
static iLed_t Led1;
led_creat(&Led1,led_1_on, led_1_off);
*/
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Exported types ------------------------------------------------------------*/
typedef struct iLed_t
{
  uint8_t state;                             // 信号灯当前的状态
  uint16_t *mode;                            // 用于指明当前参照闪烁数组中的第几个成员
  uint16_t modeLen;                          // 每种模式的长度
  uint16_t tickCnt;                          // tick计数
  uint16_t modeIndex;                        // 一个周期内的闪烁次数
  int32_t times;                             // 执行次数
  void (*on)(void);                          // led亮
  void (*off)(void);                         // led灭
  void (*overCallBack)(struct iLed_t *hled); // led闪烁结束回调函数
  struct iLed_t *next;                       // 下一个led灯
} iLed_t;
/* Exported constants --------------------------------------------------------*/
#define LED_TICK_TIME 1                     // 心跳函数调用的时间间隔(单位:ms)
/* Exported macro ------------------------------------------------------------*/

/* Exported variables ---------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
extern void led_tick_loop(void);
extern void led_loop(void);
extern void led_tick(void);

extern int8_t led_creat(iLed_t *handle, void (*on)(void), void (*off)(void));
extern void led_set_mode(iLed_t *handle, uint16_t *mode, uint16_t modeLen, int16_t times);
#endif /* __DRV_LED_H */
