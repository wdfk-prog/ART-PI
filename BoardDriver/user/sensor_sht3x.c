/**
 * @File Name: sensor_sht3x.c
 * @brief  
 * @Author : 
 * @Version : 1.0
 * @Creat Date : 2024-01-25
 * 
 * @copyright Copyright (c) 2024 
 * @par 修改日志:
 * Date           Version     Author  Description
 * 2024-01-25     v1.0        huagnly 内容
*/
/* Includes ------------------------------------------------------------------*/
#include "sensor_sht3x.h"
/* Private includes ----------------------------------------------------------*/
#include "i2c.h"
/*ulog include*/
#define LOG_TAG              "sht3x"
#define LOG_LVL              DBG_INFO
#include <ulog.h>
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static void sht3x_reset(I2C_HandleTypeDef *hi2c, sht3x_power_t *power);
static void sht3x_delay(uint32_t delay);
static bool sht3x_i2c_read(I2C_HandleTypeDef *hi2c, uint8_t *data, uint16_t datasize);
static bool sht3x_i2c_write(I2C_HandleTypeDef *hi2c, uint8_t *data, uint16_t datasize);

static sht3x_driver_cfg_t sht3x_cfg[SHT3X_MAX_NUM] =
{
    [SHT3X_0] = 
    {
        .device = 
        {
            .hi2c       = &hi2c4,
            .reset      = sht3x_reset,
            .delay_ms   = sht3x_delay,
            .iic_read   = sht3x_i2c_read,
            .iic_write  = sht3x_i2c_write,
        },
        .i2c_init = MX_I2C4_Init,
    },
};

sht3x_device_t sht3x[SHT3X_MAX_NUM] = 
{
    [SHT3X_0] = 
    {
        .cfg = &sht3x_cfg[SHT3X_0], //配置信息
    },
};
/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
/**************************************************
 * @brief 系统延时ms
 **************************************************/
static void sht3x_delay(uint32_t delay)
{
    rt_thread_delay(delay);
}
/**************************************************
 * @brief 重置sht3x设备
 **************************************************/
static void sht3x_reset(I2C_HandleTypeDef *hi2c, sht3x_power_t *power)
{
    // HAL_I2C_DeInit(hi2c);
    // sht3x_delay(10);
    // HAL_I2C_Init(hi2c);
    // sht3x_delay(10);
}
/**************************************************
 * @brief IIC读设备
 **************************************************/
static bool sht3x_i2c_read(I2C_HandleTypeDef *hi2c, uint8_t *data, uint16_t datasize)
{
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(hi2c, (0x44<<1 | 1), data, datasize, 100);
    if (status != HAL_OK) {
        LOG_E("[sht3x]:read failed 0x%02x", status);
        return false;
    } else {
        return true;
    }
}
/**************************************************
 * @brief IIC写设备
 **************************************************/
static bool sht3x_i2c_write(I2C_HandleTypeDef *hi2c, uint8_t *data, uint16_t datasize)
{
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hi2c, (0x44<<1 | 0), data, datasize, 100);
    if (status != HAL_OK) {
        LOG_E("[sht3x]:write failed 0x%02x", status);
        return false;
    } else {
        return true;
    }
}

/**
 * @brief  获取 sth3x 数据
 * @param  None
 * @retval int
 * @note   None
 */
int sht3x_get(void)
{
    sht3x_driver_cfg_t *config = &sht3x_cfg[0];
    sht3x_device_t *dev = &sht3x[0];
    MX_I2C4_Init();
    sht3x_init(&config->device);

    if(sht3x_collect_process(&config->device) == false) {
    } else {
        sht3x_get_current_temp(&config->device, &config->raw[SHT3X_DATA_TEMPERATURE]);
        sht3x_get_current_humi(&config->device, &config->raw[SHT3X_DATA_HUMIDITY]);
        LOG_I("temp raw:%f", config->raw[SHT3X_DATA_TEMPERATURE]);
        LOG_I("humi raw:%f", config->raw[SHT3X_DATA_HUMIDITY]);
    }

    return RT_EOK;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(sht3x_get, get sht3x);