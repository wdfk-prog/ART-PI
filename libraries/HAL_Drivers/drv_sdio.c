/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-05-23     liuduanfei   first version
 */
#include "board.h"

#ifdef RT_USING_SDIO

#if !defined(BSP_USING_SDIO1) && !defined(BSP_USING_SDIO2)
    #error "Please define at least one BSP_USING_SDIOx"
#endif

#include "drv_sdio.h"

#define DBG_TAG              "drv.sdio"
#ifdef DRV_DEBUG
    #define DBG_LVL               DBG_LOG
#else
    #define DBG_LVL               DBG_INFO
#endif /* DRV_DEBUG */
#include <rtdbg.h>

static struct stm32_sdio_class sdio_obj;
static struct rt_mmcsd_host *host1;
static struct rt_mmcsd_host *host2;

#define SDIO_TX_RX_COMPLETE_TIMEOUT_LOOPS    (1000000)

#define RTHW_SDIO_LOCK(_sdio)   rt_mutex_take(&_sdio->mutex, RT_WAITING_FOREVER)
#define RTHW_SDIO_UNLOCK(_sdio) rt_mutex_release(&_sdio->mutex);

struct sdio_pkg
{
    struct rt_mmcsd_cmd *cmd;
    void *buff;
    rt_uint32_t flag;
};

struct rthw_sdio
{
    struct rt_mmcsd_host *host;
    struct stm32_sdio_des sdio_des;
    struct rt_event event;
    struct rt_mutex mutex;
    struct sdio_pkg *pkg;
};

rt_align(SDIO_ALIGN_LEN)
static rt_uint8_t cache_buf[SDIO_BUFF_SIZE];

static rt_uint32_t stm32_sdio_clk_get(struct stm32_sdio *hw_sdio)
{
    return SDIO_CLOCK_FREQ;
}

/**
  * @brief  This function get order from sdio.
  * @param  data
  * @retval sdio order
  */
static int get_order(rt_uint32_t data)
{
    int order = 0;

    switch (data)
    {
        case 1:
            order = 0;
            break;

        case 2:
            order = 1;
            break;

        case 4:
            order = 2;
            break;

        case 8:
            order = 3;
            break;

        case 16:
            order = 4;
            break;

        case 32:
            order = 5;
            break;

        case 64:
            order = 6;
            break;

        case 128:
            order = 7;
            break;

        case 256:
            order = 8;
            break;

        case 512:
            order = 9;
            break;

        case 1024:
            order = 10;
            break;

        case 2048:
            order = 11;
            break;

        case 4096:
            order = 12;
            break;

        case 8192:
            order = 13;
            break;

        case 16384:
            order = 14;
            break;

        default :
            order = 0;
            break;
    }

    return order;
}

/**
  * @brief  This function wait sdio cmd completed.
  * @param  sdio rthw_sdio
  * @retval None
  */
static void rthw_sdio_wait_completed(struct rthw_sdio *sdio)
{
    rt_uint32_t status;
    struct rt_mmcsd_cmd *cmd = sdio->pkg->cmd;
    struct rt_mmcsd_data *data = cmd->data;
    struct stm32_sdio *hw_sdio = sdio->sdio_des.hw_sdio;

    if (rt_event_recv(&sdio->event, 0xffffffff, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      rt_tick_from_millisecond(5000), &status) != RT_EOK)
    {
        LOG_E("wait cmd completed timeout");
        cmd->err = -RT_ETIMEOUT;
        return;
    }

    if (sdio->pkg == RT_NULL)
    {
        return;
    }

    cmd->resp[0] = hw_sdio->resp1;

    if (resp_type(cmd) == RESP_R2)
    {
        cmd->resp[1] = hw_sdio->resp2;
        cmd->resp[2] = hw_sdio->resp3;
        cmd->resp[3] = hw_sdio->resp4;
    }

    if (status & HW_SDIO_ERRORS)
    {
        if ((status & HW_SDIO_IT_CCRCFAIL) && (resp_type(cmd) & (RESP_R3 | RESP_R4)))
        {
            cmd->err = RT_EOK;
        }
        else
        {
            cmd->err = -RT_ERROR;
        }

        if (status & HW_SDIO_IT_CTIMEOUT)
        {
            cmd->err = -RT_ETIMEOUT;
        }
        if (status & HW_SDIO_IT_DCRCFAIL)
        {
            data->err = -RT_ERROR;
        }

        if (status & HW_SDIO_IT_DTIMEOUT)
        {
            data->err = -RT_ETIMEOUT;
        }

        if (cmd->err == RT_EOK)
        {
            LOG_D("sta:0x%08X [%08X %08X %08X %08X]", status, cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);
        }
        else
        {
            LOG_D("err:0x%08x, %s%s%s%s%s%s%s cmd:%d arg:0x%08x rw:%c len:%d blksize:%d",
                  status,
                  status & HW_SDIO_IT_CCRCFAIL  ? "CCRCFAIL "    : "",
                  status & HW_SDIO_IT_DCRCFAIL  ? "DCRCFAIL "    : "",
                  status & HW_SDIO_IT_CTIMEOUT  ? "CTIMEOUT "    : "",
                  status & HW_SDIO_IT_DTIMEOUT  ? "DTIMEOUT "    : "",
                  status & HW_SDIO_IT_TXUNDERR  ? "TXUNDERR "    : "",
                  status & HW_SDIO_IT_RXOVERR   ? "RXOVERR "     : "",
                  status == 0                   ? "NULL"         : "",
                  cmd->cmd_code,
                  cmd->arg,
                  data ? (data->flags & DATA_DIR_WRITE ?  'w' : 'r') : '-',
                  data ? data->blks * data->blksize : 0,
                  data ? data->blksize : 0
                 );
        }
    }
    else
    {
        cmd->err = RT_EOK;
        LOG_D("sta:0x%08X [%08X %08X %08X %08X]", status, cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);
    }
}

/**
  * @brief  This function send command.
  * @param  sdio rthw_sdio
  * @param  pkg  sdio package
  * @retval None
  */
static void rthw_sdio_send_command(struct rthw_sdio *sdio, struct sdio_pkg *pkg)
{
    struct rt_mmcsd_cmd *cmd = pkg->cmd;
    struct rt_mmcsd_data *data = cmd->data;
    struct stm32_sdio *hw_sdio = sdio->sdio_des.hw_sdio;
    rt_uint32_t reg_cmd;

    rt_event_control(&sdio->event, RT_IPC_CMD_RESET, RT_NULL);
    /* save pkg */
    sdio->pkg = pkg;

    LOG_D("CMD:%d ARG:0x%08x RES:%s%s%s%s%s%s%s%s%s rw:%c len:%d blksize:%d\n",
          cmd->cmd_code,
          cmd->arg,
          resp_type(cmd) == RESP_NONE ? "NONE"  : "",
          resp_type(cmd) == RESP_R1  ? "R1"  : "",
          resp_type(cmd) == RESP_R1B ? "R1B"  : "",
          resp_type(cmd) == RESP_R2  ? "R2"  : "",
          resp_type(cmd) == RESP_R3  ? "R3"  : "",
          resp_type(cmd) == RESP_R4  ? "R4"  : "",
          resp_type(cmd) == RESP_R5  ? "R5"  : "",
          resp_type(cmd) == RESP_R6  ? "R6"  : "",
          resp_type(cmd) == RESP_R7  ? "R7"  : "",
          data ? (data->flags & DATA_DIR_WRITE ?  'w' : 'r') : '-',
          data ? data->blks * data->blksize : 0,
          data ? data->blksize : 0
         );

    hw_sdio->mask |= SDIO_MASKR_ALL;
    reg_cmd = cmd->cmd_code | SDMMC_CMD_CPSMEN;

    /* data pre configuration */
    if (data != RT_NULL)
    {
        SCB_CleanInvalidateDCache();

        reg_cmd |= SDMMC_CMD_CMDTRANS;
        hw_sdio->mask &= ~(SDMMC_MASK_CMDRENDIE | SDMMC_MASK_CMDSENTIE);
        hw_sdio->dtimer = HW_SDIO_DATATIMEOUT;
        hw_sdio->dlen = data->blks * data->blksize;
        hw_sdio->dctrl = (get_order(data->blksize) << 4) | (data->flags & DATA_DIR_READ ? SDMMC_DCTRL_DTDIR : 0);
        hw_sdio->idmabase0r = (rt_uint32_t)cache_buf;
        hw_sdio->idmatrlr = SDMMC_IDMA_IDMAEN;
    }

    if (resp_type(cmd) == RESP_R2)
        reg_cmd |= SDMMC_CMD_WAITRESP;
    else if(resp_type(cmd) != RESP_NONE)
        reg_cmd |= SDMMC_CMD_WAITRESP_0;

    hw_sdio->arg = cmd->arg;
    hw_sdio->cmd = reg_cmd;
    /* wait completed */
    rthw_sdio_wait_completed(sdio);

    /* Waiting for data to be sent to completion */
    if (data != RT_NULL)
    {
        volatile rt_uint32_t count = SDIO_TX_RX_COMPLETE_TIMEOUT_LOOPS;

        while (count && (hw_sdio->sta & SDMMC_STA_DPSMACT))
        {
            count--;
        }

        if ((count == 0) || (hw_sdio->sta & HW_SDIO_ERRORS))
        {
            cmd->err = -RT_ERROR;
        }
    }

    /* data post configuration */
    if (data != RT_NULL)
    {
        if (data->flags & DATA_DIR_READ)
        {
            rt_memcpy(data->buf, cache_buf, data->blks * data->blksize);
            SCB_CleanInvalidateDCache();
        }
    }
}

/**
  * @brief  This function send sdio request.
  * @param  sdio  rthw_sdio
  * @param  req   request
  * @retval None
  */
static void rthw_sdio_request(struct rt_mmcsd_host *host, struct rt_mmcsd_req *req)
{
    struct sdio_pkg pkg;
    struct rthw_sdio *sdio = host->private_data;
    struct rt_mmcsd_data *data;

    RTHW_SDIO_LOCK(sdio);

    if (req->cmd != RT_NULL)
    {
        rt_memset(&pkg, 0, sizeof(pkg));
        data = req->cmd->data;
        pkg.cmd = req->cmd;

        if (data != RT_NULL)
        {
            rt_uint32_t size = data->blks * data->blksize;

            pkg.buff = data->buf;
            if ((rt_uint32_t)data->buf & (SDIO_ALIGN_LEN - 1))
            {
                pkg.buff = cache_buf;
                if (data->flags & DATA_DIR_WRITE)
                {
                    rt_memcpy(cache_buf, data->buf, size);
                }
            }
        }

        rthw_sdio_send_command(sdio, &pkg);

        if ((data != RT_NULL) && (data->flags & DATA_DIR_READ) && ((rt_uint32_t)data->buf & (SDIO_ALIGN_LEN - 1)))
        {
            rt_memcpy(data->buf, cache_buf, data->blksize * data->blks);
        }
    }

    if (req->stop != RT_NULL)
    {
        rt_memset(&pkg, 0, sizeof(pkg));
        pkg.cmd = req->stop;
        rthw_sdio_send_command(sdio, &pkg);
    }

    RTHW_SDIO_UNLOCK(sdio);

    mmcsd_req_complete(sdio->host);
}

/**
  * @brief  This function config sdio.
  * @param  host    rt_mmcsd_host
  * @param  io_cfg  rt_mmcsd_io_cfg
  * @retval None
  */
static void rthw_sdio_iocfg(struct rt_mmcsd_host *host, struct rt_mmcsd_io_cfg *io_cfg)
{
    rt_uint32_t clkcr, temp, clk_src;
    rt_uint32_t clk = io_cfg->clock;
    struct rthw_sdio *sdio = host->private_data;
    struct stm32_sdio *hw_sdio = sdio->sdio_des.hw_sdio;

    clk_src = sdio->sdio_des.clk_get(sdio->sdio_des.hw_sdio);
    if (clk_src < 400 * 1000)
    {
        LOG_E("The clock rate is too low! rata:%d", clk_src);
        return;
    }

    if (clk > host->freq_max) clk = host->freq_max;

    if (clk > clk_src)
    {
        LOG_W("Setting rate is greater than clock source rate.");
        clk = clk_src;
    }

    LOG_D("clk:%d width:%s%s%s power:%s%s%s",
          clk,
          io_cfg->bus_width == MMCSD_BUS_WIDTH_8 ? "8" : "",
          io_cfg->bus_width == MMCSD_BUS_WIDTH_4 ? "4" : "",
          io_cfg->bus_width == MMCSD_BUS_WIDTH_1 ? "1" : "",
          io_cfg->power_mode == MMCSD_POWER_OFF ? "OFF" : "",
          io_cfg->power_mode == MMCSD_POWER_UP ? "UP" : "",
          io_cfg->power_mode == MMCSD_POWER_ON ? "ON" : ""
         );

    RTHW_SDIO_LOCK(sdio);
    
//    rt_uint32_t div;
//    div = clk_src / clk;
//    if ((clk == 0) || (div == 0))
//    {
//        clkcr = 0;
//    }
//    else
//    {
//        if (div < 2)
//        {
//            div = 2;
//        }
//        else if (div > 0xFF)
//        {
//            div = 0xFF;
//        }
//        div -= 2;
//        clkcr = div | HW_SDIO_CLK_ENABLE;
//    }

//    if (io_cfg->bus_width == MMCSD_BUS_WIDTH_8)
//    {
//        clkcr |= HW_SDIO_BUSWIDE_8B;
//    }
//    else if (io_cfg->bus_width == MMCSD_BUS_WIDTH_4)
//    {
//        clkcr |= HW_SDIO_BUSWIDE_4B;
//    }
//    else
//    {
//        clkcr |= HW_SDIO_BUSWIDE_1B;
//    }

//    hw_sdio->clkcr = clkcr;

    clk_src = SDIO_CLOCK_FREQ;

    if (clk > 0)
    {
        if (clk > host->freq_max)
            clk = host->freq_max;

        temp = DIV_ROUND_UP(clk_src, 2 * clk);

        if (temp > 0x3FF)
            temp = 0x3FF;
    }

    if (io_cfg->bus_width == MMCSD_BUS_WIDTH_4)
        temp |= SDMMC_CLKCR_WIDBUS_0;
    else if (io_cfg->bus_width == MMCSD_BUS_WIDTH_8)
        temp |= SDMMC_CLKCR_WIDBUS_1;

    hw_sdio->clkcr = temp;

    if (io_cfg->power_mode == MMCSD_POWER_ON)
        hw_sdio->power |= SDMMC_POWER_PWRCTRL;

//    switch (io_cfg->power_mode)
//    {
//    case MMCSD_POWER_OFF:
//        hw_sdio->power = HW_SDIO_POWER_OFF;
//        break;
//    case MMCSD_POWER_UP:
//        hw_sdio->power = HW_SDIO_POWER_UP;
//        break;
//    case MMCSD_POWER_ON:
//        hw_sdio->power = HW_SDIO_POWER_ON;
//        break;
//    default:
//        LOG_W("unknown power_mode %d", io_cfg->power_mode);
//        break;
//    }

    RTHW_SDIO_UNLOCK(sdio);
}

/**
  * @brief  This function update sdio interrupt.
  * @param  host    rt_mmcsd_host
  * @param  enable
  * @retval None
  */
void rthw_sdio_irq_update(struct rt_mmcsd_host *host, rt_int32_t enable)
{
    struct rthw_sdio *sdio = host->private_data;
    struct stm32_sdio *hw_sdio = sdio->sdio_des.hw_sdio;

    if (enable)
    {
        LOG_D("enable sdio irq");
        hw_sdio->mask |= HW_SDIO_IT_SDIOIT;
    }
    else
    {
        LOG_D("disable sdio irq");
        hw_sdio->mask &= ~HW_SDIO_IT_SDIOIT;
    }
}

/**
  * @brief  This function detect sdcard.
  * @param  host    rt_mmcsd_host
  * @retval 0x01
  */
static rt_int32_t rthw_sd_detect(struct rt_mmcsd_host *host)
{
    LOG_D("try to detect device");
    return 0x01;
}

/**
  * @brief  This function interrupt process function.
  * @param  host  rt_mmcsd_host
  * @retval None
  */
void rthw_sdio_irq_process(struct rt_mmcsd_host *host)
{
    struct rthw_sdio *sdio = host->private_data;
    struct stm32_sdio *hw_sdio = sdio->sdio_des.hw_sdio;
    rt_uint32_t intstatus = hw_sdio->sta;

    /* clear irq flag*/
    hw_sdio->icr = intstatus;

    rt_event_send(&sdio->event, intstatus);
}
static const struct rt_mmcsd_host_ops ops =
{
    rthw_sdio_request,
    rthw_sdio_iocfg,
    rthw_sd_detect,
    rthw_sdio_irq_update,
};

/**
  * @brief  This function create mmcsd host.
  * @param  sdio_des stm32_sdio_des
  * @retval rt_mmcsd_host
  */
struct rt_mmcsd_host *sdio_host_create(struct stm32_sdio_des *sdio_des)
{
    struct rt_mmcsd_host *host;
    struct rthw_sdio *sdio = RT_NULL;

    if (sdio_des == RT_NULL)
    {
        LOG_E("L:%d F:%s",(sdio_des == RT_NULL ? "sdio_des is NULL" : ""));
        return RT_NULL;
    }

    sdio = rt_malloc(sizeof(struct rthw_sdio));

    if (sdio == RT_NULL)
    {
        LOG_E("L:%d F:%s malloc rthw_sdio fail");
        return RT_NULL;
    }
    rt_memset(sdio, 0, sizeof(struct rthw_sdio));

    host = mmcsd_alloc_host();
    if (host == RT_NULL)
    {
        LOG_E("L:%d F:%s mmcsd alloc host fail");
        rt_free(sdio);
        return RT_NULL;
    }

    rt_memcpy(&sdio->sdio_des, sdio_des, sizeof(struct stm32_sdio_des));
#ifdef BSP_USING_SDIO1
    if((SDCARD_INSTANCE_TYPE *)sdio_des->hw_sdio == SDMMC1)
    {
        sdio->sdio_des.hw_sdio = (sdio_des->hw_sdio == RT_NULL ? (struct stm32_sdio *)SDIO1_BASE_ADDRESS : sdio_des->hw_sdio);
        sdio->sdio_des.clk_get = (sdio_des->clk_get == RT_NULL ? stm32_sdio_clk_get : sdio_des->clk_get);
        rt_event_init(&sdio->event, "sdio1", RT_IPC_FLAG_FIFO);
        rt_mutex_init(&sdio->mutex, "sdio1", RT_IPC_FLAG_PRIO);
    }
#endif /* BSP_USING_SDIO1 */
#ifdef BSP_USING_SDIO2
    if((SDCARD_INSTANCE_TYPE *)sdio_des->hw_sdio == SDMMC1)
    {
        sdio->sdio_des.hw_sdio = (sdio_des->hw_sdio == RT_NULL ? (struct stm32_sdio *)SDIO2_BASE_ADDRESS : sdio_des->hw_sdio);
        sdio->sdio_des.clk_get = (sdio_des->clk_get == RT_NULL ? stm32_sdio_clk_get : sdio_des->clk_get);
        rt_event_init(&sdio->event, "sdio2", RT_IPC_FLAG_FIFO);
        rt_mutex_init(&sdio->mutex, "sdio2", RT_IPC_FLAG_PRIO);
    }
#endif /* BSP_USING_SDIO2 */

    /* set host default attributes */
    host->ops = &ops;
    host->freq_min = 400 * 1000;
    host->freq_max = SDIO_MAX_FREQ;
    host->valid_ocr = VDD_32_33 | VDD_33_34;/* The voltage range supported is 3.2v-3.4v */
#ifndef SDIO_USING_1_BIT
    host->flags = MMCSD_BUSWIDTH_4 | MMCSD_MUTBLKWRITE | MMCSD_SUP_HIGHSPEED;
#else
    host->flags = MMCSD_MUTBLKWRITE | MMCSD_SUP_SDIO_IRQ;
#endif
    host->max_seg_size = SDIO_BUFF_SIZE;
    host->max_dma_segs = 1;
    host->max_blk_size = 512;
    host->max_blk_count = 512;

    /* link up host and sdio */
    sdio->host = host;
    host->private_data = sdio;

    rthw_sdio_irq_update(host, 1);

    /* ready to change */
    mmcsd_change(host);

    return host;
}
/**
  * @brief  This function get stm32 sdio clock.
  * @param  hw_sdio: stm32_sdio
  * @retval PCLK2Freq
  */
static rt_uint32_t stm32_sdio_clock_get(struct stm32_sdio *hw_sdio)
{
    return HAL_RCC_GetPCLK2Freq();
}

void SDIO_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();
    /* Process All SDIO Interrupt Sources */
    rthw_sdio_irq_process(host1);

    /* leave interrupt */
    rt_interrupt_leave();
}

void SDMMC1_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();
    /* Process All SDIO Interrupt Sources */
    rthw_sdio_irq_process(host1);
    /* leave interrupt */
    rt_interrupt_leave();
}

void SDMMC2_IRQHandler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();
    /* Process All SDIO Interrupt Sources */
    rthw_sdio_irq_process(host2);
    /* leave interrupt */
    rt_interrupt_leave();
}

int rt_hw_sdio_init(void)
{
#ifdef BSP_USING_SDIO1
    struct stm32_sdio_des sdio_des1;
    SD_HandleTypeDef hsd1;
    hsd1.Instance = SDCARD1_INSTANCE;
    {
        rt_uint32_t tmpreg = 0x00U;
#if defined(SOC_SERIES_STM32F1)
        /* enable DMA clock && Delay after an RCC peripheral clock enabling*/
        SET_BIT(RCC->AHBENR, sdio_config.dma_rx.dma_rcc);
        tmpreg = READ_BIT(RCC->AHBENR, sdio_config.dma_rx.dma_rcc);
#elif defined(SOC_SERIES_STM32F4) || defined(SOC_SERIES_STM32F7) || defined(SOC_SERIES_STM32L4) || defined(SOC_SERIES_STM32F2)
   || defined(SOC_SERIES_STM32H7)
        SET_BIT(RCC->AHB1ENR, sdio_config.dma_rx.dma_rcc);
        /* Delay after an RCC peripheral clock enabling */
        tmpreg = READ_BIT(RCC->AHB1ENR, sdio_config.dma_rx.dma_rcc);
#endif /* defined(SOC_SERIES_STM32F1) */
        UNUSED(tmpreg); /* To avoid compiler warnings */
    }
    HAL_SD_MspInit(&hsd1);
    HAL_NVIC_SetPriority(SDCARD1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(SDCARD1_IRQn);

    sdio_des1.clk_get  = stm32_sdio_clock_get;
    sdio_des1.hw_sdio  = (struct stm32_sdio *)SDCARD1_INSTANCE;

    host1 = sdio_host_create(&sdio_des1);

    if (host1 == RT_NULL)
    {
        LOG_E("host1 create fail");
        return -1;
    }
#endif /* BSP_USING_SDIO1 */

#ifdef BSP_USING_SDIO2
    struct stm32_sdio_des sdio_des1;
    SD_HandleTypeDef hsd2;
    hsd2.Instance = SDCARD1_INSTANCE;
    {
        rt_uint32_t tmpreg = 0x00U;
#if defined(SOC_SERIES_STM32F1)
        /* enable DMA clock && Delay after an RCC peripheral clock enabling*/
        SET_BIT(RCC->AHBENR, sdio_config.dma_rx.dma_rcc);
        tmpreg = READ_BIT(RCC->AHBENR, sdio_config.dma_rx.dma_rcc);
#elif defined(SOC_SERIES_STM32F4) || defined(SOC_SERIES_STM32F7) || defined(SOC_SERIES_STM32L4) || defined(SOC_SERIES_STM32F2)
   || defined(SOC_SERIES_STM32H7)
        SET_BIT(RCC->AHB1ENR, sdio_config.dma_rx.dma_rcc);
        /* Delay after an RCC peripheral clock enabling */
        tmpreg = READ_BIT(RCC->AHB1ENR, sdio_config.dma_rx.dma_rcc);
#endif /* defined(SOC_SERIES_STM32F1) */
        UNUSED(tmpreg); /* To avoid compiler warnings */
    }
    HAL_NVIC_SetPriority(SDCARD2_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(SDCARD2_IRQn);
    HAL_SD_MspInit(&hsd2);

    sdio_des2.clk_get  = stm32_sdio_clock_get;
    sdio_des2.hw_sdio  = (struct stm32_sdio *)SDCARD2_INSTANCE;
    sdio_des2.rxconfig = DMA_RxConfig;
    sdio_des2.txconfig = DMA_TxConfig;

    host1 = sdio_host_create(&sdio_des2);

    if (host2 == RT_NULL)
    {
        LOG_E("host2 create fail");
        return -1;
    }
#endif /* BSP_USING_SDIO2 */

    return 0;
}
INIT_DEVICE_EXPORT(rt_hw_sdio_init);

void stm32_mmcsd_change(void)
{
    mmcsd_change(host1);
// mmcsd_change(host2);
}

#endif /* RT_USING_SDIO */
