/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2012-04-25     weety         first version
 * 2021-04-20     RiceChen      added support for bus control api
 * 2024-06-23     wdfk-prog     Add the config struct
 */

#ifndef __I2C_CORE_H__
#define __I2C_CORE_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RT_I2C_WR                0x0000
#define RT_I2C_RD               (1u << 0)
#define RT_I2C_ADDR_10BIT       (1u << 2)  /* this is a ten bit chip address */
#define RT_I2C_NO_START         (1u << 4)
#define RT_I2C_IGNORE_NACK      (1u << 5)
#define RT_I2C_NO_READ_ACK      (1u << 6)  /* when I2C reading, we do not ACK */
#define RT_I2C_NO_STOP          (1u << 7)

#define RT_I2C_CTRL_SET_MAX_HZ  0x20

struct rt_i2c_msg
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    rt_uint16_t len;
    rt_uint8_t  *buf;
};

struct rt_i2c_bus_device;

struct rt_i2c_bus_device_ops
{
    rt_ssize_t (*master_xfer)(struct rt_i2c_bus_device *bus,
                             struct rt_i2c_msg msgs[],
                             rt_uint32_t num);
    rt_ssize_t (*slave_xfer)(struct rt_i2c_bus_device *bus,
                            struct rt_i2c_msg msgs[],
                            rt_uint32_t num);
    rt_err_t (*i2c_bus_control)(struct rt_i2c_bus_device *bus,
                                int cmd,
                                void *args);
};

/**
 * I2C configuration structure.
 * mode : master: 0x00; slave: 0x01;
 * max_hz: Maximum limit baud rate.
 * usage_freq: Actual usage baud rate.
 */
struct rt_i2c_configuration
{
    rt_uint8_t  mode;
    rt_uint8_t  reserved[3];

    rt_uint32_t max_hz;
    rt_uint32_t usage_freq;
};

/*for i2c bus driver*/
struct rt_i2c_bus_device
{
    struct rt_device parent;
    const struct rt_i2c_bus_device_ops *ops;
    rt_uint16_t  flags;
    struct rt_mutex lock;
    rt_uint32_t  timeout;
    rt_uint32_t  retries;
    struct rt_i2c_configuration config;
    void *priv;
};

struct rt_i2c_client
{
#ifdef RT_USING_DM
    struct rt_device parent;

    const char *name;
    const struct rt_i2c_device_id *id;
    const struct rt_ofw_node_id *ofw_id;
#endif
    struct rt_i2c_bus_device       *bus;
    rt_uint16_t                    client_addr;
};

#ifdef RT_USING_DM
struct rt_i2c_device_id
{
    char name[20];
    void *data;
};

struct rt_i2c_driver
{
    struct rt_driver parent;

    const struct rt_i2c_device_id *ids;
    const struct rt_ofw_node_id *ofw_ids;

    rt_err_t (*probe)(struct rt_i2c_client *client);
    rt_err_t (*remove)(struct rt_i2c_client *client);
    rt_err_t (*shutdown)(struct rt_i2c_client *client);
};

rt_err_t rt_i2c_driver_register(struct rt_i2c_driver *driver);
rt_err_t rt_i2c_device_register(struct rt_i2c_client *client);

#define RT_I2C_DRIVER_EXPORT(driver)  RT_DRIVER_EXPORT(driver, i2c, BUILIN)
#endif /* RT_USING_DM */

rt_err_t rt_i2c_bus_device_register(struct rt_i2c_bus_device *bus,
                                    const char               *bus_name);
struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *bus_name);
rt_ssize_t rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg         msgs[],
                          rt_uint32_t               num);
rt_err_t rt_i2c_control(struct rt_i2c_bus_device *bus,
                        int cmd,
                        void *args);
rt_ssize_t rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             const rt_uint8_t         *buf,
                             rt_uint32_t               count);
rt_ssize_t rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             rt_uint8_t               *buf,
                             rt_uint32_t               count);

rt_inline rt_err_t rt_i2c_bus_lock(struct rt_i2c_bus_device *bus, rt_tick_t timeout)
{
    return rt_mutex_take(&bus->lock, timeout);
}

rt_inline rt_err_t rt_i2c_bus_unlock(struct rt_i2c_bus_device *bus)
{
    return rt_mutex_release(&bus->lock);
}

#ifdef __cplusplus
}
#endif

#endif