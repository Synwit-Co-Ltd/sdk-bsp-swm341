/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-02-01     Rudy Lo      The first version
 */

#include <lvgl.h>
#include <stdbool.h>
#include <rtdevice.h>
#include <drv_gpio.h>

#include "gt9147.h"

#define TOUCH_DEVICE_NAME    "gt9147"    /* Touch device name */
#define TOUCH_DEVICE_I2C_BUS "i2c0"      /* SCL -> PA1(1), SDA -> PA0(0) */

static rt_device_t ts;    /* Touch device handle, Touchscreen */
static struct rt_touch_data *read_data;

static rt_int16_t last_x = 0;
static rt_int16_t last_y = 0;

static bool touchpad_is_pressed(void)
{
    if (1 == rt_device_read(ts, 0, read_data, 1))
    {
        if ((read_data->event == RT_TOUCH_EVENT_DOWN) || (read_data->event == RT_TOUCH_EVENT_MOVE)) {

            /* restore data */
            last_x = read_data->x_coordinate;
            last_y = read_data->y_coordinate;

            // rt_kprintf("touch: x = %d, y = %d\n", last_x, last_y);
            return true;
        }
    }

    return false;
}

static void touchpad_get_xy(rt_int16_t *x, rt_int16_t *y)
{
    *x = last_x;
    *y = last_y;
}

static void touchpad_read(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    /*`touchpad_is_pressed` and `touchpad_get_xy` needs to be implemented by you*/
    if(touchpad_is_pressed()) {
        data->state = LV_INDEV_STATE_PRESSED;
        touchpad_get_xy(&data->point.x, &data->point.y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

rt_err_t rt_hw_gt9147_register(void)
{
    struct rt_touch_config config;
    rt_uint8_t rst;

    rst = GT9147_RST_PIN;
    config.dev_name = TOUCH_DEVICE_I2C_BUS;
    config.irq_pin.pin = GT9147_IRQ_PIN;
    config.irq_pin.mode = PIN_MODE_INPUT_PULLDOWN;
    config.user_data = &rst;
    rt_hw_gt9147_init(TOUCH_DEVICE_NAME, &config);

    ts = rt_device_find(TOUCH_DEVICE_NAME);
    if (!ts) {
        return -RT_ERROR;
    }

    read_data = (struct rt_touch_data *)rt_calloc(1, sizeof(struct rt_touch_data));
    if (!read_data) {
        return -RT_ENOMEM;
    }

    if (!rt_device_open(ts, RT_DEVICE_FLAG_RDONLY)) {
        struct rt_touch_info info;
        rt_device_control(ts, RT_TOUCH_CTRL_GET_INFO, &info);
        rt_kprintf("type       :%d\n", info.type);
        rt_kprintf("vendor     :%s\n", info.vendor);
        rt_kprintf("point_num  :%d\n", info.point_num);
        rt_kprintf("range_x    :%d\n", info.range_x);
        rt_kprintf("range_y    :%d\n", info.range_y);
        return RT_EOK;
    } else {
        rt_kprintf("open touch device failed.\n");
        return -RT_ERROR;
    }
}

lv_indev_t * touch_indev;

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;         /* Descriptor of a input device driver */
    lv_indev_drv_init(&indev_drv);           /* Basic initialization */
    indev_drv.type = LV_INDEV_TYPE_POINTER;  /* Touch pad is a pointer-like device */
    indev_drv.read_cb = touchpad_read;       /* Set your driver function */

    /* Register the driver in LVGL and save the created input device object */
    touch_indev = lv_indev_drv_register(&indev_drv);

    /* Register touch device */
    rt_hw_gt9147_register();
}
