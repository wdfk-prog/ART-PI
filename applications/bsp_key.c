/**
 * @file bsp_key.c
 * @brief 
 * @author HLY (1425075683@qq.com)
 * @version 1.0
 * @date 2022-12-02
 * @copyright Copyright (c) 2022
 * @attention 
 * @par 修改日志:
 * Date       Version Author  Description
 * 2022-12-02 1.0     HLY     first version
 */
/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
#include <ipc/ringbuffer.h>
#include <rtthread.h>
#include <board.h>
#include "mfbd.h"
#include "main.h"
/*ulog include*/
#define LOG_TAG              "bsp key" 
#define LOG_LVL              DBG_INFO
#include <ulog.h>
/* Private typedef -----------------------------------------------------------*/
/* 依次定义GPIO */
typedef struct
{
	GPIO_TypeDef* gpio;
	uint16_t pin;
	uint8_t ActiveLevel;	/* 激活电平 */
}X_GPIO_T;
/* 按键ID*/
typedef enum
{
  KID_KEY1 = 0X00,
  HARD_KEY_NUM,
}KEY_ID;
/*
	定义键值代码, 必须按如下次序定时每个键的按下、弹起和长按事件

	推荐使用enum, 不用#define，原因：
	(1) 便于新增键值,方便调整顺序，使代码看起来舒服点
	(2) 编译器可帮我们避免键值重复。
*/
typedef enum
{
	KEY_NONE = 0X00,			      /* 0 表示按键事件 */

  KEY1_DOWN_CODE,				   
  KEY1_UP_CODE,				    
  KEY1_LONG_CODE,				   
}KEY_CODE;
/* GPIO和PIN定义 */
static const X_GPIO_T s_gpio_list[HARD_KEY_NUM] = 
{
	{KEY1_GPIO_Port,           KEY1_Pin,          GPIO_PIN_RESET},	  /* 按键1按钮*/
};	
/* Private define ------------------------------------------------------------*/
/* 扫描线程配置 */
#define SCAN_THREAD_PRIORITY      28//线程优先级
#define SCAN_THREAD_TIMESLICE     10 //线程时间片
#define SCAN_THREAD_STACK_SIZE    512//栈大小
/* 处理线程配置 */
#define HANDLER_THREAD_PRIORITY   29//线程优先级
#define HANDLER_THREAD_TIMESLICE  10 //线程时间片
#define HANDLER_THREAD_STACK_SIZE 1024//栈大小

#define RINGBUFFER_LEN       10//ringbuffer缓冲区大小
#define SCAN_PERIOD          10//扫描周期 单位ms
#define HANDLER_PERIOD       50//处理周期 单位ms
/* Private macro -------------------------------------------------------------*/
/*支持Button单击、长按的操作，不支持多次连击操作*/
#if MFBD_USE_NORMAL_BUTTON//重复时间与长按时间请大于滤波时间
/*                       名称,             按键索引,          滤波时间,        重复时间，     长按时间        */
MFBD_NBTN_DEFAULT_DEFINE(KEY1,            KID_KEY1);//持续读取
#endif /* MFBD_USE_NORMAL_BUTTON */
#if MFBD_USE_NORMAL_BUTTON
MFBD_NBTN_ARRAYLIST(nbtn_list,
                    &KEY1    );
#endif /* MFBD_USE_NORMAL_BUTTON */
/* Private variables ---------------------------------------------------------*/
static void bsp_btn_value_report(mfbd_btn_code_t btn_value);
static unsigned char bsp_btn_check(mfbd_btn_index_t btn_index);
//环形缓冲区指针
struct rt_ringbuffer * rb;
const mfbd_group_t btn_group =
{
    bsp_btn_check,
    bsp_btn_value_report,

#if MFBD_USE_TINY_BUTTON
    tbtn_list,
#endif /* MFBD_USE_TINY_BUTTON */

#if MFBD_USE_NORMAL_BUTTON
    nbtn_list,
#endif /* MFBD_USE_NORMAL_BUTTON */

#if MFBD_USE_MULTIFUCNTION_BUTTON
    mbtn_list,
#endif /* MFBD_USE_MULTIFUCNTION_BUTTON */

#if MFBD_PARAMS_SAME_IN_GROUP

#if MFBD_USE_TINY_BUTTON || MFBD_USE_NORMAL_BUTTON || MFBD_USE_MULTIFUCNTION_BUTTON
    50/SCAN_PERIOD,                                //滤波时间
#endif /*  MFBD_USE_TINY_BUTTON || MFBD_USE_NORMAL_BUTTON || MFBD_USE_MULTIFUCNTION_BUTTON */

#if MFBD_USE_NORMAL_BUTTON || MFBD_USE_MULTIFUCNTION_BUTTON
    300/SCAN_PERIOD,                               //重复时间
    1500/SCAN_PERIOD,                              //长按时间
#endif /* MFBD_USE_NORMAL_BUTTON || MFBD_USE_MULTIFUCNTION_BUTTON */

#if MFBD_USE_MULTIFUCNTION_BUTTON
    750/SCAN_PERIOD,                              //多次点击时间
#endif /* MFBD_USE_MULTIFUCNTION_BUTTON */

#endif /*MFBD_PARAMS_SAME_IN_GROUP*/

};
/* Private function prototypes -----------------------------------------------*/
/**
 * @brief  按键处理函数
 * @param  ucKeyCode        
 */
void key_handle(mfbd_btn_code_t ucKeyCode)
{
  if (ucKeyCode != KEY_NONE)
  {
    switch (ucKeyCode)
    {
      case KEY1_UP_CODE:
        LOG_I("KEY1 UP");
        break;
      case KEY1_DOWN_CODE: 
        LOG_I("KEY1 DOWN");
        break;
        break;
      case KEY1_LONG_CODE:
        LOG_I("KEY1 LONG");
        break;
      default:
        /* 其它的键值不处理 */
        break;
    }
  }
}
/******************************按键驱动********************************************/
/**
  * @brief  KeyPinActive.
  * @param  None.
  * @retval 返回值1 表示按下(导通），0表示未按下（释放）.
  * @note   判断按键是否按下.
*/
static uint8_t key_pin_active(mfbd_btn_index_t _id)
{
	uint8_t level;
	
	if ((s_gpio_list[_id].gpio->IDR & s_gpio_list[_id].pin) == 0)
	{
		level = 0;
	}
	else
	{
		level = 1;
	}

	if (level == s_gpio_list[_id].ActiveLevel)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
/**
  * @brief  按键检测.
  * @param  None.
  * @retval None.
  * @note   STM32 HAL库专用.
*/
static unsigned char bsp_btn_check(mfbd_btn_index_t btn_index)
{
    return key_pin_active(btn_index);
}
/**
  * @brief  键值反馈.
  * @param  None.
  * @retval None.
  * @note   None
*/
static void bsp_btn_value_report(mfbd_btn_code_t btn_value)
{
  LOG_D("%d", btn_value);
  //该接口会继续写入剩余的数据，即用新的数据覆盖旧的数据。
  rt_ringbuffer_put_force(rb,(rt_uint8_t *)&btn_value,sizeof(mfbd_btn_code_t));
}
/**
  * @brief  按键扫描线程.
  * @param  None.
  * @retval None.
  * @note   None
*/
static void mfbd_scan(void *arg)
{
  while (1)
  {
    mfbd_group_scan(&btn_group); /* scan button group */
    rt_thread_mdelay(SCAN_PERIOD); /* scan period: 10ms */
  }
}
/**
  * @brief  按键处理线程.
  * @param  None.
  * @retval None.
  * @note   None
*/
static void mfbd_handler(void *p)
{
  while(1)
  {
    mfbd_btn_code_t recv_data;
    rt_size_t recv = rt_ringbuffer_get(rb, (rt_uint8_t *)&recv_data, sizeof(recv_data));
    if(recv == sizeof(recv_data))
      key_handle(recv_data);    
    rt_thread_mdelay(50); /* handler period: 50ms */
  }
}
/**
  * @brief  按键初始化
  * @param  None.
  * @retval None.
  * @note   None
*/
static void user_button_init(void)
{
  //MX_GPIO_Init();//已在borad中初始化
}
/**
  * @brief  按键线程初始化
  * @param  None.
  * @retval None.
  * @note   None
*/
int mfbd_init(void)
{
  rt_err_t ret = RT_EOK;
  user_button_init();
                                                  //length	缓冲区字节大小
  rb = rt_ringbuffer_create(sizeof(mfbd_btn_code_t) * RINGBUFFER_LEN);
  RT_ASSERT(rb != RT_NULL);

  /* 创建线程*/
  rt_thread_t thread = rt_thread_create( "mfbdscan",    /* 线程名字 */
                                         mfbd_scan,/* 线程入口函数 */
                                         RT_NULL,       /* 线程入口函数参数 */
                                         SCAN_THREAD_STACK_SIZE, /* 线程栈大小 */
                                         SCAN_THREAD_PRIORITY,   /* 线程的优先级 */
                                         SCAN_THREAD_TIMESLICE); /* 线程时间片 */
  /* 创建成功则启动线程 */
  if (thread != RT_NULL)
  {
      rt_thread_startup(thread);
  }
  else
  {
      ret = RT_ERROR;
      LOG_E("mf scan created failed.");
  }
  /* 创建线程*/
  thread = rt_thread_create( "mfdbhandler",    /* 线程名字 */
                            mfbd_handler,       /* 线程入口函数 */
                            RT_NULL,            /* 线程入口函数参数 */
                    HANDLER_THREAD_STACK_SIZE,  /* 线程栈大小 */
                    HANDLER_THREAD_PRIORITY,    /* 线程的优先级 */
                    HANDLER_THREAD_TIMESLICE);  /* 线程时间片 */
  /* 创建成功则启动线程 */
  if (thread != RT_NULL)
  {
      rt_thread_startup(thread);
  }
  else
  {
      ret = RT_ERROR;
      LOG_E("mf hand created failed.");
  }
  return ret;
}
INIT_APP_EXPORT(mfbd_init);