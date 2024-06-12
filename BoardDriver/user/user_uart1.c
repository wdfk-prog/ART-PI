/**
 * @file user_uart1.c
 * @brief
 * @author HLY (1425075683@qq.com)
 * @version 1.0
 * @date 2022-12-26
 * @copyright Copyright (c) 2022
 * @attention
 * @par �޸���־:
 * Date       Version Author  Description
 * 2022-12-26 1.0     HLY     first version
 */
/* Includes ------------------------------------------------------------------*/
#include <rtthread.h>
#include <rtdevice.h>
/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* ���ڽ�����Ϣ�ṹ */
struct rx_msg
{
    rt_device_t dev;
    rt_size_t size;
};
/* Private define ------------------------------------------------------------*/
#define UART_NAME "uart1" /* �����豸���� */
/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/* �����豸��� */
static rt_device_t serial;
/* ��Ϣ���п��ƿ� */
static struct rt_messagequeue rx_mq;
/* Private function prototypes -----------------------------------------------*/
/**
 * @brief  ���ڽ��պ���
 * @param  dev
 * @param  size
 * @retval rt_err_t
 */
static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
{
    struct rx_msg msg;
    rt_err_t result;
    msg.dev = dev;
    msg.size = size;

    result = rt_mq_send(&rx_mq, &msg, sizeof(msg));
    if (result == -RT_EFULL)
    {
        /* ��Ϣ������ */
        rt_kprintf("message queue full��\n");
    }
    return result;
}
/**
 * @brief ���ڽ����߳�
 * @param  parameter
 */
static void serial_thread_entry(void *parameter)
{
    struct rx_msg msg;
    rt_err_t result;
    rt_uint32_t rx_length;
    static char rx_buffer[BSP_UART1_RX_BUFSIZE + 1];

    while (1)
    {
        rt_memset(&msg, 0, sizeof(msg));
        /* ����Ϣ�����ж�ȡ��Ϣ */
        result = rt_mq_recv(&rx_mq, &msg, sizeof(msg), RT_WAITING_FOREVER);
        if (result == RT_EOK)
        {
            /* �Ӵ��ڶ�ȡ���� */
            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);
            rx_buffer[rx_length] = '\0';
            /* ͨ�������豸 serial �����ȡ������Ϣ */
            rt_device_write(serial, 0, rx_buffer, rx_length);
            /* ��ӡ���� */
            rt_kprintf("%s\n", rx_buffer);
        }
    }
}
/**
 * @brief ����1��ʼ��
 * @retval int
 */
static int uart1_init(void)
{
    rt_err_t ret = RT_EOK;
    static char msg_pool[256];
    char str[] = "hello RT-Thread!\r\n";

    /* ���Ҵ����豸 */
    serial = rt_device_find(UART_NAME);
    if (!serial)
    {
        rt_kprintf("find %s failed!\n", UART_NAME);
        return RT_ERROR;
    }

    /* ��ʼ����Ϣ���� */
    rt_mq_init(&rx_mq, "rx_mq",
               msg_pool,              /* �����Ϣ�Ļ����� */
               sizeof(struct rx_msg), /* һ����Ϣ����󳤶� */
               sizeof(msg_pool),      /* �����Ϣ�Ļ�������С */
               RT_IPC_FLAG_FIFO);     /* ����ж���̵߳ȴ������������ȵõ��ķ���������Ϣ */

    /* �� DMA ���ռ���ѯ���ͷ�ʽ�򿪴����豸 */
    rt_device_open(serial, RT_DEVICE_FLAG_RX_NON_BLOCKING | RT_DEVICE_FLAG_TX_BLOCKING);
    /* ���ý��ջص����� */
    rt_device_set_rx_indicate(serial, uart_input);
    /* �����ַ��� */
    rt_device_write(serial, 0, str, (sizeof(str) - 1));

    /* ���� serial �߳� */
    rt_thread_t thread = rt_thread_create("uart1", serial_thread_entry, RT_NULL, 1024, 25, 10);
    /* �����ɹ��������߳� */
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
    }
    else
    {
        ret = RT_ERROR;
    }

    return ret;
}
INIT_APP_EXPORT(uart1_init);