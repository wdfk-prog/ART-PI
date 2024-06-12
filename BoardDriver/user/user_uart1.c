///**
// * @file user_uart1.c
// * @brief
// * @author HLY (1425075683@qq.com)
// * @version 1.0
// * @date 2022-12-26
// * @copyright Copyright (c) 2022
// * @attention
// * @par 修改日志:
// * Date       Version Author  Description
// * 2022-12-26 1.0     HLY     first version
// */
///* Includes ------------------------------------------------------------------*/
//#include <rtthread.h>
//#include <rtdevice.h>
///* Private includes ----------------------------------------------------------*/

///* Private typedef -----------------------------------------------------------*/
///* 串口接收消息结构 */
//struct rx_msg
//{
//    rt_device_t dev;
//    rt_size_t size;
//};
///* Private define ------------------------------------------------------------*/
//#define UART_NAME "uart1" /* 串口设备名称 */
///* Private macro -------------------------------------------------------------*/

///* Private variables ---------------------------------------------------------*/
///* 串口设备句柄 */
//static rt_device_t serial;
///* 消息队列控制块 */
//static struct rt_messagequeue rx_mq;
///* Private function prototypes -----------------------------------------------*/
///**
// * @brief  串口接收函数
// * @param  dev
// * @param  size
// * @retval rt_err_t
// */
//static rt_err_t uart_input(rt_device_t dev, rt_size_t size)
//{
//    struct rx_msg msg;
//    rt_err_t result;
//    msg.dev = dev;
//    msg.size = size;

//    result = rt_mq_send(&rx_mq, &msg, sizeof(msg));
//    if (result == -RT_EFULL)
//    {
//        /* 消息队列满 */
//        rt_kprintf("message queue full！\n");
//    }
//    return result;
//}
///**
// * @brief 串口接收线程
// * @param  parameter
// */
//static void serial_thread_entry(void *parameter)
//{
//    struct rx_msg msg;
//    rt_err_t result;
//    rt_uint32_t rx_length;
//    static char rx_buffer[BSP_UART1_RX_BUFSIZE + 1];

//    while (1)
//    {
//        rt_memset(&msg, 0, sizeof(msg));
//        /* 从消息队列中读取消息 */
//        result = rt_mq_recv(&rx_mq, &msg, sizeof(msg), RT_WAITING_FOREVER);
//        if (result == RT_EOK)
//        {
//            /* 从串口读取数据 */
//            rx_length = rt_device_read(msg.dev, 0, rx_buffer, msg.size);
//            rx_buffer[rx_length] = '\0';
//            /* 通过串口设备 serial 输出读取到的消息 */
//            rt_device_write(serial, 0, rx_buffer, rx_length);
//            /* 打印数据 */
//            rt_kprintf("%s\n", rx_buffer);
//        }
//    }
//}
///**
// * @brief 串口1初始化
// * @retval int
// */
//static int uart1_init(void)
//{
//    rt_err_t ret = RT_EOK;
//    static char msg_pool[256];
//    char str[] = "hello RT-Thread!\r\n";

//    /* 查找串口设备 */
//    serial = rt_device_find(UART_NAME);
//    if (!serial)
//    {
//        rt_kprintf("find %s failed!\n", UART_NAME);
//        return RT_ERROR;
//    }

//    /* 初始化消息队列 */
//    rt_mq_init(&rx_mq, "rx_mq",
//               msg_pool,              /* 存放消息的缓冲区 */
//               sizeof(struct rx_msg), /* 一条消息的最大长度 */
//               sizeof(msg_pool),      /* 存放消息的缓冲区大小 */
//               RT_IPC_FLAG_FIFO);     /* 如果有多个线程等待，按照先来先得到的方法分配消息 */

//    /* 以 DMA 接收及轮询发送方式打开串口设备 */
//    rt_device_open(serial, RT_DEVICE_FLAG_RX_NON_BLOCKING | RT_DEVICE_FLAG_TX_BLOCKING);
//    /* 设置接收回调函数 */
//    rt_device_set_rx_indicate(serial, uart_input);
//    /* 发送字符串 */
//    rt_device_write(serial, 0, str, (sizeof(str) - 1));

//    /* 创建 serial 线程 */
//    rt_thread_t thread = rt_thread_create("uart1", serial_thread_entry, RT_NULL, 1024, 25, 10);
//    /* 创建成功则启动线程 */
//    if (thread != RT_NULL)
//    {
//        rt_thread_startup(thread);
//    }
//    else
//    {
//        ret = RT_ERROR;
//    }

//    return ret;
//}
//INIT_APP_EXPORT(uart1_init);