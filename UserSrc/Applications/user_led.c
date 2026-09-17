/*
 * @file user_led.c
 * @brief
 * @author wdfk-prog (1425075683@qq.com)
 * @version 1.0
 * @date 2023-02-01
 *
 * @copyright Copyright (c) 2023
 *
 * @attention
 * @par �޸���־:
 * Date       Version Author  Description
 * 2023-02-01 1.0     wdfk-prog    first version
 */
/* Includes ------------------------------------------------------------------*/

/* Private includes ----------------------------------------------------------*/
#include "drv_gpio.h"
#include "drv_led.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
// ����ʱ�� = 500 -�� 500ms
uint16_t LED_MODE_1[] = {500, 500};                               // ����ģʽ����500 ��500
uint16_t LED_MODE_2[] = {200, 200, 200, 200, 200, 200, 200, 500}; // �Ĵ���˸����
uint16_t LED_MODE_3[] = {200, 200, 200, 200, 200, 500};           // ������˸����
uint16_t LED_MODE_4[] = {100, 100};                               // ������˸
uint16_t LED_MODE_5[] = {200, 200, 100, 500};                     // ������˸����
uint16_t LED_MODE_6[] = {0, 200};                                 // ����
/* Private macro -------------------------------------------------------------*/
#define LED_MODE(x) x, sizeof(x)
/* Private variables ---------------------------------------------------------*/
static iLed_t Led1;
static rt_timer_t timer1;
/* Private function prototypes -----------------------------------------------*/
void Led_CallBack(iLed_t *handle);
/**
 * @brief  ��һ
 * @param  None
 * @retval None
 * @note   None
 */
void led_set()
{
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
}
/**
 * @brief  ����
 * @param  None
 * @retval None
 * @note   None
 */
void led_reset(void)
{
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
}
/**
 * @brief  ��ʱ����ʱ�ص�
 * @note   led������ִ��
 * @param  *parameter:
 * @retval None
 */
static void timeout1(void *parameter)
{
    led_tick();
    led_tick_loop();
}
/**
 * @brief  �û���ʼ��LED
 * @param  None.
 * @retval None.
 * @note
 */
static int User_Led_Init(void)
{
    led_creat(&Led1, led_set, led_reset);
    led_set_mode(&Led1, LED_MODE(LED_MODE_1), -1);

    /* ������ʱ�� 1  ���ڶ�ʱ�� */
    timer1 = rt_timer_create("timer1",
                             timeout1,
                             RT_NULL,
                             1,
                             RT_TIMER_FLAG_PERIODIC);
    /* ������ʱ�� 1 */
    if (timer1 != RT_NULL)
        rt_timer_start(timer1);
    return 0;
}
INIT_APP_EXPORT(User_Led_Init);
