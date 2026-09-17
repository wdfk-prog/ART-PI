/**
 * ****************************************************************************
 * @file SHT3X 驱动文件
 * @note 
 * 
 * ****************************************************************************
*/
#include <stdio.h>
#include <string.h>
#include "sht3x.h"
#include "rtthread.h"
static bool SHT3X_GetTempAndHumi(sht3x_handle_t *dev, uint16_t mode);
static void sht3x_avg_calculate(sht3x_handle_t *dev);
static uint8_t SHT3X_CalcCrc(uint8_t data[], uint8_t nbrOfBytes);


/**************************************************
 * @brief SHT3X 初始化
 * @param[in] dev SHT3X 句柄
 **************************************************/
void sht3x_init(sht3x_handle_t *dev)
{
    bool ret = false;
    if (dev != NULL && dev->iic_read != NULL && dev->iic_write != NULL && dev->delay_ms != NULL && dev->reset != NULL) {
        // 清空状态 
        dev->state = SHT3X_INITED;
        dev->attempt = false;
        dev->error_count = 0;
        memset(&dev->temp_data, 0, sizeof(dev->temp_data));
        memset(&dev->humi_data, 0, sizeof(dev->humi_data));
        // 尝试采集三次
        for (uint8_t i = 0; i < 3; i++) {
            dev->reset(dev->hi2c, &dev->power);
            ret = SHT3X_GetTempAndHumi(dev, CMD_MEAS_POLLING_L);
            if (ret == true) {
                break;
            }
        }
        if (ret == true) {
            sht3x_avg_calculate(dev);
            rt_kprintf("[sht3x]init successful! temp = %d, humi = %d\r\n",
                    (int16_t)dev->temp_data.CurValue, (int8_t)dev->humi_data.CurValue);
            return;
        } else {
            // 置位尝试标志以期恢复通信
            dev->attempt = true;
        }
    }
    rt_kprintf("[sht3x]:init failed!\r\n");
}


/**************************************************
 * @brief SHT3X 数据采集流程
 * @return 采集结果
 **************************************************/
bool sht3x_collect_process(sht3x_handle_t *dev)
{
    bool ret = false;

    // 未定义或者设备损坏返回失败
    if (dev == NULL || dev->state != SHT3X_INITED) {
        return false;
    }

    if (dev->attempt == true) {
        // 只尝试采集一次
        ret = SHT3X_GetTempAndHumi(dev, CMD_MEAS_POLLING_L);
    } else {
        // 尝试采集三次，都失败时认为采集失败
        for (uint8_t i = 0; i < 3; i++) {
            ret = SHT3X_GetTempAndHumi(dev, CMD_MEAS_POLLING_L);
            if (ret == true) {
                break;
            } else {
                dev->reset(dev->hi2c, &dev->power);
            }
        }
    }
    // 采集成功则清除失败计数
    if (ret == true) {
        sht3x_avg_calculate(dev);
        dev->error_count = 0;
        dev->attempt = false;
    } else {
        // 连续8次采集失败设置损坏标志
        if (dev->attempt == false) {
            dev->error_count++;
        }
        if (dev->error_count > 8) {
            dev->state = SHT3X_BROKEN;
        }
    }

    return ret;
}

/**************************************************
 * @brief SHT3X 获取当前温度
 * @param[out] temp 温度值
 * @return 采集结果
 **************************************************/
bool sht3x_get_current_temp(sht3x_handle_t *dev, float *temp)
{
    if (dev == NULL || dev->state != SHT3X_INITED || dev->attempt == true || dev->error_count != 0) {
        return false;
    } else {
        *temp = dev->temp_data.CurValue;
        return true;
    }
}

/**************************************************
 * @brief SHT3X 获取当前湿度
 * @param[out] humi 湿度值
 * @return 采集结果
 **************************************************/
bool sht3x_get_current_humi(sht3x_handle_t *dev, float *humi)
{
    if (dev == NULL || dev->state != SHT3X_INITED || dev->attempt == true || dev->error_count != 0) {
        return false;
    } else {
        *humi = dev->humi_data.CurValue;
        return true;
    }
}

/**************************************************
 * @brief SHT3X 根据模式获取温度和湿度数据
 * @param[in] mode 采集模式
 * @return 获取结果
 **************************************************/
static bool SHT3X_GetTempAndHumi(sht3x_handle_t *dev, uint16_t mode)
{
    if (dev == NULL || dev->state != SHT3X_INITED) {
        return false;
    }

    uint8_t read_mode[2] = {0};
    uint8_t SHT3X_READ_BUF[6] = {0};
    uint8_t temp_data[2] = {0};
    uint8_t temp_check = 0;
    uint8_t humi_data[2] = {0};
    uint8_t humi_check = 0;

    read_mode[0] = (mode >> 8);
    read_mode[1] = mode & 0xFF;

    if (dev->iic_write(dev->hi2c, read_mode, 2) != true) {
        return false;
    }

    dev->delay_ms(10);

    if (dev->iic_read(dev->hi2c, SHT3X_READ_BUF, 6) != true) {
        return false;
    }

    temp_data[0] = SHT3X_READ_BUF[0];
    temp_data[1] = SHT3X_READ_BUF[1];
    temp_check = SHT3X_READ_BUF[2];

    humi_data[0] = SHT3X_READ_BUF[3];
    humi_data[1] = SHT3X_READ_BUF[4];
    humi_check = SHT3X_READ_BUF[5];

    if ((SHT3X_CalcCrc(temp_data, 2) == temp_check) && (SHT3X_CalcCrc(humi_data, 2) == humi_check)) {
        uint16_t temp_value = temp_data[0] << 8 | temp_data[1];
        uint16_t humi_value = humi_data[0] << 8 | humi_data[1];
        dev->temp_data.CurValue = 175.0f * (float)temp_value / 65535.0f - 45.0f;
        dev->humi_data.CurValue = 100.0f * (float)humi_value / 65535.0f;
        return true;
    } else {
        return false;
    }
}

/**************************************************
 * @brief SHT3X 平均值计算
 **************************************************/
static void sht3x_avg_calculate(sht3x_handle_t *dev)
{
    if (dev == NULL || dev->state != SHT3X_INITED) {
        return;
    }

    dev->temp_data.SumValue += dev->temp_data.CurValue;
    dev->temp_data.count++;
    dev->temp_data.AvgValue = dev->temp_data.SumValue / dev->temp_data.count;

    dev->humi_data.SumValue += dev->humi_data.CurValue;
    dev->humi_data.count++;
    dev->humi_data.AvgValue = dev->humi_data.SumValue / dev->humi_data.count;
}

/**************************************************
 * @brief SHT3X 使用的 CRC 校验
 * @param[in] data 需要校验的数据
 * @param[in] nbrofBytes 校验数据的长度
 **************************************************/
static uint8_t SHT3X_CalcCrc(uint8_t data[], uint8_t nbrOfBytes)
{
    uint8_t crc = 0xFF;

    for (uint8_t byteCtr = 0; byteCtr < nbrOfBytes; byteCtr++) {
        crc ^= (data[byteCtr]);
        for (uint8_t bit = 8; bit > 0; bit--) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x131;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return crc;
}
