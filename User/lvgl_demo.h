/**
 ****************************************************************************************************
 * @file        lvgl_demo.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-01-11
 * @brief       LVGL 内部字库读取 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 F407电机开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */
 
#ifndef __LVGL_DEMO_H
#define __LVGL_DEMO_H

#include <stdint.h>

typedef enum
{
    MACHINE_UI_CMD_NONE = 0,
    MACHINE_UI_CMD_AUTO_START,
    MACHINE_UI_CMD_AUTO_STOP,
    MACHINE_UI_CMD_MODE_SINGLE,
    MACHINE_UI_CMD_MODE_CONTINUOUS,
    MACHINE_UI_CMD_MANUAL_HOME_X,
    MACHINE_UI_CMD_MANUAL_HOME_Y,
    MACHINE_UI_CMD_MANUAL_HOME_Z,
    MACHINE_UI_CMD_PARAM_SAVE
} machine_ui_cmd_type_t;

typedef struct
{
    machine_ui_cmd_type_t type;
    int32_t value;
} machine_ui_cmd_t;


void lvgl_demo(void);   /* lvgl例程 */
uint8_t machine_ui_send_cmd(machine_ui_cmd_type_t type, int32_t value);

#endif
