/**
 * @file bsp_key.c
 * @brief 
 * @author HLY (1425075683@qq.com)
 * @version 1.0
 * @date 2022-12-02
 * @copyright Copyright (c) 2022
 * @attention 
 * @par �޸���־:
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
/* ���ζ���GPIO */
typedef struct
{
	GPIO_TypeDef* gpio;
	uint16_t pin;
	uint8_t ActiveLevel;	/* �����ƽ */
}X_GPIO_T;
/* ����ID */
typedef enum
{
  KID_KEY1 = 0X00,
  HARD_KEY_NUM,
}KEY_ID;
/*
  �����ε��״̬
*/
typedef enum
{
  KEY_NONE,
  KEY1_CLICK_1,
  KEY1_CLICK_2,
  KEY1_CLICK_3,
  MAX_CLICK_NUM,
}CLICK_CODE;
/*	
  �����ֵ����,���밴���´���ʱÿ�����İ��¡�����ͳ����¼�
	�Ƽ�ʹ��enum, ����#define��ԭ��
	(1) ����������ֵ,�������˳��ʹ���뿴���������
	(2) �������ɰ����Ǳ����ֵ�ظ���
*/
typedef enum
{
  KEY1_UP_CODE = MAX_CLICK_NUM,
  KEY1_LONG_CODE,
}KEY_CODE;
/* GPIO��PIN���� */
static const X_GPIO_T s_gpio_list[HARD_KEY_NUM] = 
{
	{KEY1_GPIO_Port,           KEY1_Pin,          GPIO_PIN_RESET},	  /* ����1��ť*/
};	
/* Private define ------------------------------------------------------------*/
/* ɨ���߳����� */
#define SCAN_THREAD_PRIORITY      28//�߳����ȼ�
#define SCAN_THREAD_TIMESLICE     10 //�߳�ʱ��Ƭ
#define SCAN_THREAD_STACK_SIZE    512//ջ��С
/* �����߳����� */
#define HANDLER_THREAD_PRIORITY   29//�߳����ȼ�
#define HANDLER_THREAD_TIMESLICE  10 //�߳�ʱ��Ƭ
#define HANDLER_THREAD_STACK_SIZE 1024//ջ��С

#define RINGBUFFER_LEN       10//ringbuffer��������С
#define SCAN_PERIOD          10//ɨ������ ��λms
#define HANDLER_PERIOD       50//�������� ��λms
/* Private macro -------------------------------------------------------------*/
const mfbd_btn_code_t KEY1_DOWN_CODES[MAX_CLICK_NUM - 1] = 
{KEY1_CLICK_1,KEY1_CLICK_2,KEY1_CLICK_3};
/*֧��Button����&�����Ĳ���,�����������*/
#if MFBD_USE_MULTIFUCNTION_BUTTON//�ظ�ʱ���볤��ʱ��������˲�ʱ��
/*                       ����,             ��������, ��������� */
MFBD_MBTN_DEFAULT_DEFINE(KEY1,            KID_KEY1,  MAX_CLICK_NUM - 1);//������ȡ
#endif /* MFBD_USE_NORMAL_BUTTON */
#if MFBD_USE_MULTIFUCNTION_BUTTON
MFBD_MBTN_ARRAYLIST(mbtn_list,
                    &KEY1    );
#endif /* MFBD_USE_NORMAL_BUTTON */
/* Private variables ---------------------------------------------------------*/
static void bsp_btn_value_report(mfbd_btn_code_t btn_value);
static unsigned char bsp_btn_check(mfbd_btn_index_t btn_index);
//���λ�����ָ��
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
    50/SCAN_PERIOD,                                //�˲�ʱ��
#endif /*  MFBD_USE_TINY_BUTTON || MFBD_USE_NORMAL_BUTTON || MFBD_USE_MULTIFUCNTION_BUTTON */

#if MFBD_USE_NORMAL_BUTTON || MFBD_USE_MULTIFUCNTION_BUTTON
    2000/SCAN_PERIOD,                              //�ظ�ʱ��
    1000/SCAN_PERIOD,                              //����ʱ��
#endif /* MFBD_USE_NORMAL_BUTTON || MFBD_USE_MULTIFUCNTION_BUTTON */

#if MFBD_USE_MULTIFUCNTION_BUTTON
    300/SCAN_PERIOD,                               //��ε��ʱ��
#endif /* MFBD_USE_MULTIFUCNTION_BUTTON */

#endif /*MFBD_PARAMS_SAME_IN_GROUP*/

};
/* Private function prototypes -----------------------------------------------*/
/**
 * @brief  ����������
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
      case KEY1_CLICK_1: 
        LOG_I("KEY1 DOWN");
        break;
      case KEY1_LONG_CODE:
        LOG_I("KEY1 LONG");
        break;
      case KEY1_CLICK_2: 
        LOG_I("KEY1 CLICK 2");
        break;
      case KEY1_CLICK_3: 
        LOG_I("KEY1 CLICK 3");
        break;
      default:
        /* �����ļ�ֵ������ */
        LOG_I("KEY1 %d",ucKeyCode);
        break;
    }
  }
}
/******************************��������********************************************/
/**
  * @brief  KeyPinActive.
  * @param  None.
  * @retval ����ֵ1 ��ʾ����(��ͨ����0��ʾδ���£��ͷţ�.
  * @note   �жϰ����Ƿ���.
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
  * @brief  �������.
  * @param  None.
  * @retval None.
  * @note   STM32 HAL��ר��.
*/
static unsigned char bsp_btn_check(mfbd_btn_index_t btn_index)
{
    return key_pin_active(btn_index);
}
/**
  * @brief  ��ֵ����.
  * @param  None.
  * @retval None.
  * @note   None
*/
static void bsp_btn_value_report(mfbd_btn_code_t btn_value)
{
  LOG_D("%d", btn_value);
  //�ýӿڻ����д��ʣ������ݣ������µ����ݸ��Ǿɵ����ݡ�
  rt_ringbuffer_put_force(rb,(rt_uint8_t *)&btn_value,sizeof(mfbd_btn_code_t));
}
/**
  * @brief  ����ɨ���߳�.
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
  * @brief  ���������߳�.
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
  * @brief  ������ʼ��
  * @param  None.
  * @retval None.
  * @note   None
*/
static void user_button_init(void)
{
  //MX_GPIO_Init();//����borad�г�ʼ��
}
/**
  * @brief  �����̳߳�ʼ��
  * @param  None.
  * @retval None.
  * @note   None
*/
int mfbd_init(void)
{
  rt_err_t ret = RT_EOK;
  user_button_init();
                                                  //length	�������ֽڴ�С
  rb = rt_ringbuffer_create(sizeof(mfbd_btn_code_t) * RINGBUFFER_LEN);
  RT_ASSERT(rb != RT_NULL);

  /* �����߳�*/
  rt_thread_t thread = rt_thread_create( "mfbdscan",    /* �߳����� */
                                         mfbd_scan,/* �߳���ں��� */
                                         RT_NULL,       /* �߳���ں������� */
                                         SCAN_THREAD_STACK_SIZE, /* �߳�ջ��С */
                                         SCAN_THREAD_PRIORITY,   /* �̵߳����ȼ� */
                                         SCAN_THREAD_TIMESLICE); /* �߳�ʱ��Ƭ */
  /* �����ɹ��������߳� */
  if (thread != RT_NULL)
  {
      rt_thread_startup(thread);
  }
  else
  {
      ret = RT_ERROR;
      LOG_E("mf scan created failed.");
  }
  /* �����߳�*/
  thread = rt_thread_create( "mfdbhandler",    /* �߳����� */
                            mfbd_handler,       /* �߳���ں��� */
                            RT_NULL,            /* �߳���ں������� */
                    HANDLER_THREAD_STACK_SIZE,  /* �߳�ջ��С */
                    HANDLER_THREAD_PRIORITY,    /* �̵߳����ȼ� */
                    HANDLER_THREAD_TIMESLICE);  /* �߳�ʱ��Ƭ */
  /* �����ɹ��������߳� */
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
//INIT_APP_EXPORT(mfbd_init);