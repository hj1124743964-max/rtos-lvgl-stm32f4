/**
 ****************************************************************************************************
 * @file        lv_ui.c
 * @author      姝ｇ偣鍘熷瓙鍥㈤槦(ALIENTEK)
 * @version     V1.0
 * @date        2020-03-23
 * @brief       LVGL 鍐呴儴瀛楀簱璇诲彇 瀹為獙
 * @license     Copyright (c) 2020-2032, 骞垮窞甯傛槦缈肩數瀛愮鎶€鏈夐檺鍏徃
 ****************************************************************************************************
 * @attention
 *
 * 瀹為獙骞冲彴:姝ｇ偣鍘熷瓙 F407鐢垫満寮€鍙戞澘
 * 鍦ㄧ嚎瑙嗛:www.yuanzige.com
 * 鎶€鏈锟?www.openedv.com
 * 鍏徃缃戝潃:www.alientek.com
 * 璐拱鍦板潃:openedv.taobao.com
 *
 ****************************************************************************************************
 */
 
#include "LVGL/GUI_APP/lv_ui.h"
#include "lvgl.h"
#include "./BSP/RTC/MyRTC.h"
#include "lvgl_demo.h"
#include <stdio.h>
#include <stdint.h>

LV_FONT_DECLARE(lv_font_chinese_10)
LV_FONT_DECLARE(lv_font_chinese_16)
LV_FONT_DECLARE(lv_font_chinese_24)

static lv_obj_t *home_title_label;
static lv_obj_t *company_label;
static lv_obj_t *home_buttons[5];
static lv_obj_t *auto_mode_btn_label;
static lv_obj_t *target_count_textarea;
static lv_obj_t *current_count_textarea;
static lv_obj_t *num_keyboard;
static lv_obj_t *x_axis_value_label;
static lv_obj_t *y_axis_value_label;
static lv_obj_t *z_axis_value_label;
static lv_obj_t *editing_position_label;
static lv_obj_t *date_label;
static lv_obj_t *time_label;
static lv_timer_t *time_refresh_timer;
static uint32_t target_count_value = 1000;
static uint32_t current_count_value = 0;
static uint32_t x_run_speed_value;
static uint32_t y_run_speed_value;
static uint32_t z_run_speed_value;
static uint32_t x_jog_speed_value;
static uint32_t y_jog_speed_value;
static uint32_t z_jog_speed_value;
static uint32_t x_pickup_pos_value;
static uint32_t y_pickup_pos_value;
static uint32_t z_pickup_pos_value;
static uint32_t loose_claw_distance_value;
static uint32_t position_values[6][3];
static char product_name_text[32] = "";
static int32_t x_axis_value;
static int32_t y_axis_value;
static int32_t z_axis_value;
static bool auto_continuous_mode;

#define NAV_BTN_W      120
#define NAV_BTN_H      44
#define NAV_BTN_GAP    30
#define NAV_LEFT_X     18

static void home_page_create(void);
static void auto_page_create(void);
static void manual_page_create(void);
static void parameter_page_create(void);
static void position_page_create(void);
static void product_page_create(void);
static void auto_btn_event_cb(lv_event_t *e);
static void manual_btn_event_cb(lv_event_t *e);
static void parameter_btn_event_cb(lv_event_t *e);
static void position_btn_event_cb(lv_event_t *e);
static void product_btn_event_cb(lv_event_t *e);
static void position_value_event_cb(lv_event_t *e);
static void number_textarea_event_cb(lv_event_t *e);
static void product_name_event_cb(lv_event_t *e);
static void keyboard_event_cb(lv_event_t *e);
static void axis_move_event_cb(lv_event_t *e);
static void axis_home_event_cb(lv_event_t *e);
static void time_refresh_timer_cb(lv_timer_t *timer);
static void auto_start_event_cb(lv_event_t *e);
static void auto_stop_event_cb(lv_event_t *e);
static void param_save_event_cb(lv_event_t *e);
static lv_obj_t *create_text_label(lv_obj_t *parent, const char *text, const lv_font_t *font, lv_color_t color);

static void reset_page_objects(void)
{
    home_title_label = NULL;
    company_label = NULL;
    auto_mode_btn_label = NULL;
    target_count_textarea = NULL;
    current_count_textarea = NULL;
    num_keyboard = NULL;
    x_axis_value_label = NULL;
    y_axis_value_label = NULL;
    z_axis_value_label = NULL;
    editing_position_label = NULL;
    date_label = NULL;
    time_label = NULL;

    for(uint8_t i = 0; i < 5; i++) {
        home_buttons[i] = NULL;
    }
}

static void refresh_time_labels(void)
{
    char date_text[24];
    char time_text[16];

    if(date_label == NULL || time_label == NULL) {
        return;
    }

    MyRTC_ReadTime();
    snprintf(date_text, sizeof(date_text), "%04u年%02u月%02u日",
             (unsigned int)MyRTC_Time[0],
             (unsigned int)MyRTC_Time[1],
             (unsigned int)MyRTC_Time[2]);
    snprintf(time_text, sizeof(time_text), "%02u:%02u:%02u",
             (unsigned int)MyRTC_Time[3],
             (unsigned int)MyRTC_Time[4],
             (unsigned int)MyRTC_Time[5]);

    lv_label_set_text(date_label, date_text);
    lv_label_set_text(time_label, time_text);
}

static void time_header_create(lv_obj_t *parent, lv_color_t color)
{
    date_label = create_text_label(parent, "0000年00月00日", &lv_font_chinese_16, color);
    lv_obj_align(date_label, LV_ALIGN_TOP_LEFT, 28, 18);

    time_label = create_text_label(parent, "00:00:00", &lv_font_chinese_16, color);
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -28, 18);

    refresh_time_labels();
}

static void time_refresh_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    refresh_time_labels();
}

static lv_obj_t *create_text_label(lv_obj_t *parent, const char *text, const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, color, LV_STATE_DEFAULT);
    lv_label_set_text(label, text);
    return label;
}

static lv_obj_t *create_menu_button(lv_obj_t *parent, const char *text, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_radius(btn, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xFFFFFF), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_DEFAULT);

    lv_obj_t *label = create_text_label(btn, text, &lv_font_chinese_16, lv_color_hex(0x1E3A5F));
    lv_obj_center(label);

    return btn;
}

static lv_obj_t *create_blue_button(lv_obj_t *parent, const char *text, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_radius(btn, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0xBDE7EA), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(btn, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x66C4CB), LV_STATE_DEFAULT);

    lv_obj_t *label = create_text_label(btn, text, &lv_font_chinese_16, lv_color_black());
    lv_obj_center(label);

    return btn;
}

static void set_axis_value_label(lv_obj_t *label, int32_t value)
{
    char text[16];
    long integer = (long)(value / 100);
    long decimal = (long)(value % 100);

    if(decimal < 0) {
        decimal = -decimal;
    }

    snprintf(text, sizeof(text), "%ld.%02ld", integer, decimal);
    lv_label_set_text(label, text);
}

static lv_obj_t *create_axis_value(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, int32_t value)
{
    lv_obj_t *label = create_text_label(parent, "0.00", &lv_font_chinese_16, lv_color_black());
    lv_obj_set_size(label, 76, 30);
    lv_obj_set_style_border_width(label, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(label, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(label, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(label, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, x, y);
    set_axis_value_label(label, value);
    return label;
}

static uint32_t text_to_u32(const char *text)
{
    uint32_t value = 0;

    while(*text >= '0' && *text <= '9') {
        value = value * 10 + (uint32_t)(*text - '0');
        if(value > 999999) {
            return 999999;
        }
        text++;
    }

    return value;
}

static void set_textarea_number(lv_obj_t *textarea, uint32_t value)
{
    char text[12];
    snprintf(text, sizeof(text), "%lu", (unsigned long)value);
    lv_textarea_set_text(textarea, text);
}

static lv_obj_t *create_number_textarea(lv_obj_t *parent, uint32_t value)
{
    lv_obj_t *textarea = lv_textarea_create(parent);
    lv_obj_t *label = NULL;
    lv_obj_set_size(textarea, 120, 38);
    lv_obj_set_style_text_font(textarea, &lv_font_chinese_16, LV_STATE_DEFAULT);
    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_accepted_chars(textarea, "0123456789");
    lv_textarea_set_max_length(textarea, 6);
    lv_obj_clear_flag(textarea, LV_OBJ_FLAG_SCROLLABLE);
    set_textarea_number(textarea, value);
    label = lv_obj_get_child(textarea, 0);
    if(label != NULL) {
        lv_obj_set_width(label, lv_pct(100));
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }
    return textarea;
}

static lv_obj_t *create_param_textarea(lv_obj_t *parent, uint32_t *value)
{
    lv_obj_t *textarea = create_number_textarea(parent, *value);
    lv_obj_set_size(textarea, 132, 28);
    lv_obj_set_style_bg_color(textarea, lv_color_hex(0xBDE7EA), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(textarea, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(textarea, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(textarea, lv_color_hex(0x66C4CB), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(textarea, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_pad_top(textarea, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(textarea, 2, LV_PART_MAIN);
    lv_obj_set_style_pad_left(textarea, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_right(textarea, 4, LV_PART_MAIN);
    lv_obj_set_user_data(textarea, value);
    lv_obj_add_event_cb(textarea, number_textarea_event_cb, LV_EVENT_ALL, NULL);
    return textarea;
}

static lv_obj_t *create_position_value_box(lv_obj_t *parent, uint32_t *value)
{
    char text[12];
    lv_obj_t *box = lv_label_create(parent);
    lv_obj_set_size(box, 96, 36);
    lv_obj_set_style_radius(box, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(box, lv_color_hex(0xBDE7EA), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(box, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(box, lv_color_hex(0x66C4CB), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(box, &lv_font_chinese_16, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(box, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(box, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(box, 6, LV_STATE_DEFAULT);
    lv_obj_add_flag(box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(box, value);

    snprintf(text, sizeof(text), "%lu", (unsigned long)*value);
    lv_label_set_text(box, text);

    lv_obj_add_event_cb(box, position_value_event_cb, LV_EVENT_CLICKED, NULL);
    return box;
}

static lv_obj_t *create_light_button(lv_obj_t *parent, const char *text, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_size(label, w, h);
    lv_obj_set_style_radius(label, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(label, lv_color_hex(0xBDE7EA), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(label, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(label, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(label, lv_color_hex(0x66C4CB), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(label, &lv_font_chinese_16, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_black(), LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(label, 6, LV_STATE_DEFAULT);
    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_text(label, text);
    return label;
}

static void bottom_nav_create(lv_obj_t *parent)
{
    static const char *btn_texts[] = {
        "自动",
        "手动",
        "参数",
        "位置",
        "产品信息"
    };

    for(uint8_t i = 0; i < 5; i++) {
        home_buttons[i] = create_menu_button(parent, btn_texts[i], NAV_BTN_W, NAV_BTN_H);
        lv_obj_align(home_buttons[i], LV_ALIGN_BOTTOM_LEFT, NAV_LEFT_X + i * (NAV_BTN_W + NAV_BTN_GAP), -18);
			  lv_obj_set_style_bg_color(home_buttons[i], lv_color_hex(0x66C4CB), LV_STATE_DEFAULT);
    }

    lv_obj_add_event_cb(home_buttons[0], auto_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(home_buttons[1], manual_btn_event_cb, LV_EVENT_CLICKED, NULL);
		lv_obj_add_event_cb(home_buttons[2],parameter_btn_event_cb,LV_EVENT_CLICKED,NULL);
    lv_obj_add_event_cb(home_buttons[3], position_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(home_buttons[4], product_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

static void auto_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        auto_page_create();
    }
}

static void manual_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        manual_page_create();
    }
}
static void parameter_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        parameter_page_create();
    }
}

static void position_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        position_page_create();
    }
}

static void product_btn_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        product_page_create();
    }
}

static void position_value_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *box = lv_event_get_target(e);
        uint32_t *value = (uint32_t *)lv_obj_get_user_data(box);

        if(value == NULL) {
            return;
        }

        editing_position_label = box;

        lv_obj_t *textarea = create_param_textarea(lv_scr_act(), value);
        lv_obj_set_size(textarea, 160, 42);
        lv_obj_align(textarea, LV_ALIGN_CENTER, 0, -80);
        lv_obj_move_foreground(textarea);
        lv_event_send(textarea, LV_EVENT_CLICKED, NULL);
    }
}
static void current_count_zero_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        current_count_value = 0;
        set_textarea_number(current_count_textarea, current_count_value);
    }
}

static void number_textarea_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *textarea = lv_event_get_target(e);

    if(code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        if(num_keyboard == NULL) {
            num_keyboard = lv_keyboard_create(lv_scr_act());
            lv_obj_set_size(num_keyboard, LV_HOR_RES, 145);
            lv_obj_align(num_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
            lv_keyboard_set_mode(num_keyboard, LV_KEYBOARD_MODE_NUMBER);
            lv_obj_add_event_cb(num_keyboard, keyboard_event_cb, LV_EVENT_READY, NULL);
            lv_obj_add_event_cb(num_keyboard, keyboard_event_cb, LV_EVENT_CANCEL, NULL);
        }

        lv_keyboard_set_textarea(num_keyboard, textarea);
        lv_obj_clear_flag(num_keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(num_keyboard);
    }
    else if(code == LV_EVENT_VALUE_CHANGED || code == LV_EVENT_READY) {
        uint32_t value = text_to_u32(lv_textarea_get_text(textarea));
        if(textarea == target_count_textarea) {
            target_count_value = value;
        }
        else if(textarea == current_count_textarea) {
            current_count_value = value;
        }
        else {
            uint32_t *param_value = (uint32_t *)lv_obj_get_user_data(textarea);
            if(param_value != NULL) {
                *param_value = value;
                if(editing_position_label != NULL) {
                    char text[12];
                    snprintf(text, sizeof(text), "%lu", (unsigned long)value);
                    lv_label_set_text(editing_position_label, text);
                }
            }
        }
    }
}

static lv_obj_t *create_product_name_textarea(lv_obj_t *parent)
{
    lv_obj_t *textarea = lv_textarea_create(parent);
    lv_obj_set_size(textarea, 320, 62);
    lv_obj_set_style_text_font(textarea, &lv_font_chinese_16, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(textarea, lv_color_hex(0xBDE7EA), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(textarea, LV_OPA_COVER, LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(textarea, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(textarea, lv_color_hex(0x66C4CB), LV_STATE_DEFAULT);
    lv_textarea_set_one_line(textarea, true);
    lv_textarea_set_max_length(textarea, sizeof(product_name_text) - 1);
    lv_textarea_set_text(textarea, product_name_text);
    lv_obj_clear_flag(textarea, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(textarea, product_name_event_cb, LV_EVENT_ALL, NULL);
    return textarea;
}

static void product_name_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *textarea = lv_event_get_target(e);

    if(code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        if(num_keyboard == NULL) {
            num_keyboard = lv_keyboard_create(lv_scr_act());
            lv_obj_set_size(num_keyboard, LV_HOR_RES, 145);
            lv_obj_align(num_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
            lv_obj_add_event_cb(num_keyboard, keyboard_event_cb, LV_EVENT_READY, NULL);
            lv_obj_add_event_cb(num_keyboard, keyboard_event_cb, LV_EVENT_CANCEL, NULL);
        }

        lv_keyboard_set_mode(num_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
        lv_keyboard_set_textarea(num_keyboard, textarea);
        lv_obj_clear_flag(num_keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(num_keyboard);
    }
    else if(code == LV_EVENT_VALUE_CHANGED || code == LV_EVENT_READY) {
        snprintf(product_name_text, sizeof(product_name_text), "%s", lv_textarea_get_text(textarea));
    }
}

static void keyboard_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        if(num_keyboard != NULL) {
            lv_obj_t *textarea = lv_keyboard_get_textarea(num_keyboard);
            lv_obj_add_flag(num_keyboard, LV_OBJ_FLAG_HIDDEN);
            lv_keyboard_set_textarea(num_keyboard, NULL);
            if(editing_position_label != NULL && textarea != NULL) {
                lv_obj_del(textarea);
                editing_position_label = NULL;
            }
        }
    }
}

static void refresh_axis_label(uint8_t axis)
{
    if(axis == 0 && x_axis_value_label != NULL) {
        set_axis_value_label(x_axis_value_label, x_axis_value);
    }
    else if(axis == 1 && y_axis_value_label != NULL) {
        set_axis_value_label(y_axis_value_label, y_axis_value);
    }
    else if(axis == 2 && z_axis_value_label != NULL) {
        set_axis_value_label(z_axis_value_label, z_axis_value);
    }
}

static void axis_move_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
        int32_t data = (int32_t)(intptr_t)lv_event_get_user_data(e);
        uint8_t axis = (uint8_t)(data >> 8);
        int32_t direction = data & 0xFF;

        if(direction >= 128) {
            direction -= 256;
        }

        int32_t step = (code == LV_EVENT_LONG_PRESSED_REPEAT) ? 50 : 10;

        if(axis == 0) {
            x_axis_value += direction * step;
        }
        else if(axis == 1) {
            y_axis_value += direction * step;
        }
        else if(axis == 2) {
            z_axis_value += direction * step;
        }

        refresh_axis_label(axis);
    }
}

static void axis_home_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        uint8_t axis = (uint8_t)(intptr_t)lv_event_get_user_data(e);

        if(axis == 0) {
            x_axis_value = 0;
        }
        else if(axis == 1) {
            y_axis_value = 0;
        }
        else if(axis == 2) {
            z_axis_value = 0;
        }

        if(axis == 0) {
            machine_ui_send_cmd(MACHINE_UI_CMD_MANUAL_HOME_X, 0);
        }
        else if(axis == 1) {
            machine_ui_send_cmd(MACHINE_UI_CMD_MANUAL_HOME_Y, 0);
        }
        else if(axis == 2) {
            machine_ui_send_cmd(MACHINE_UI_CMD_MANUAL_HOME_Z, 0);
        }

        refresh_axis_label(axis);
    }
}

static void auto_mode_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        auto_continuous_mode = !auto_continuous_mode;
        lv_label_set_text(auto_mode_btn_label, auto_continuous_mode ? "连动" : "单动");
        machine_ui_send_cmd(auto_continuous_mode ? MACHINE_UI_CMD_MODE_CONTINUOUS : MACHINE_UI_CMD_MODE_SINGLE, 0);
    }
}

static void auto_start_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        machine_ui_send_cmd(MACHINE_UI_CMD_AUTO_START, 0);
    }
}

static void auto_stop_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        machine_ui_send_cmd(MACHINE_UI_CMD_AUTO_STOP, 0);
    }
}

static void param_save_event_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        machine_ui_send_cmd(MACHINE_UI_CMD_PARAM_SAVE, 0);
    }
}

static void home_page_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    reset_page_objects();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1E88E5), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_STATE_DEFAULT);
    time_header_create(scr, lv_color_white());

    home_title_label = create_text_label(scr, "当前主页", &lv_font_chinese_16, lv_color_white());
    lv_obj_align(home_title_label, LV_ALIGN_TOP_MID, 0, 18);

    company_label = create_text_label(scr, "乔丰科技（河源）实业有限公司", &lv_font_chinese_24, lv_color_white());
    lv_obj_center(company_label);

    bottom_nav_create(scr);
}

static void auto_page_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    reset_page_objects();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xF3F6F9), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_STATE_DEFAULT);
    time_header_create(scr, lv_color_hex(0x1E3A5F));

    lv_obj_t *title = create_text_label(scr, "当前页面自动页", &lv_font_chinese_16, lv_color_hex(0x1E3A5F));
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *yield_panel = lv_obj_create(scr);
    lv_obj_set_size(yield_panel, 300, 210);
    lv_obj_align(yield_panel, LV_ALIGN_LEFT_MID, 18, -40);
    lv_obj_set_style_radius(yield_panel, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(yield_panel, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(yield_panel, 12, LV_STATE_DEFAULT);

    lv_obj_t *yield_title = create_text_label(yield_panel, "数量", &lv_font_chinese_16, lv_color_hex(0x1E3A5F));
    lv_obj_align(yield_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *set_label = create_text_label(yield_panel, "数量设置", &lv_font_chinese_16, lv_color_hex(0x263238));
    lv_obj_align(set_label, LV_ALIGN_TOP_LEFT, 0, 45);

    target_count_textarea = create_number_textarea(yield_panel, target_count_value);
    lv_obj_align(target_count_textarea, LV_ALIGN_TOP_LEFT, 130, 38);
    lv_obj_add_event_cb(target_count_textarea, number_textarea_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *current_label = create_text_label(yield_panel, "当前数量", &lv_font_chinese_16, lv_color_hex(0x263238));
    lv_obj_align(current_label, LV_ALIGN_TOP_LEFT, 0, 100);

    current_count_textarea = create_number_textarea(yield_panel, current_count_value);
    lv_obj_align(current_count_textarea, LV_ALIGN_TOP_LEFT, 130, 93);
    lv_obj_add_event_cb(current_count_textarea, number_textarea_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *zero_btn = create_menu_button(yield_panel, "回原点", 86, 38);
    lv_obj_align(zero_btn, LV_ALIGN_TOP_LEFT, 130, 145);
    lv_obj_add_event_cb(zero_btn, current_count_zero_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *alarm_panel = lv_obj_create(scr);
    lv_obj_set_size(alarm_panel, 270, 210);
    lv_obj_align(alarm_panel, LV_ALIGN_RIGHT_MID, -18, -40);
    lv_obj_set_style_radius(alarm_panel, 6, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(alarm_panel, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(alarm_panel, 12, LV_STATE_DEFAULT);

    lv_obj_t *alarm_title = create_text_label(alarm_panel, "报警列表", &lv_font_chinese_16, lv_color_hex(0x1E3A5F));
    lv_obj_align(alarm_title, LV_ALIGN_TOP_LEFT, 0, 0);

    const char *alarm_texts[] = {
        "上轴报警",
        "下轴报警",
        "左轴报警",
        "右轴报警",
        "前轴报警",
        "后轴报警"
    };

    for(uint8_t i = 0; i < 6; i++) {
        lv_obj_t *alarm = create_text_label(alarm_panel, alarm_texts[i], &lv_font_chinese_16, lv_color_hex(0xC62828));
        lv_obj_align(alarm, LV_ALIGN_TOP_LEFT, 0, 38 + i * 26);
    }

    lv_obj_t *start_btn = create_menu_button(scr, "启动", 110, 44);
    lv_obj_align(start_btn, LV_ALIGN_BOTTOM_LEFT, 80, -100);
		lv_obj_set_style_bg_color(start_btn, lv_color_hex(0x66C4CB), LV_STATE_DEFAULT);
    lv_obj_add_event_cb(start_btn, auto_start_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *stop_btn = create_menu_button(scr, "停止", 110, 44);
    lv_obj_align(stop_btn, LV_ALIGN_BOTTOM_LEFT, 220, -100);
    lv_obj_set_style_bg_color(stop_btn, lv_color_hex(0x66C4CB), LV_STATE_DEFAULT);
    lv_obj_add_event_cb(stop_btn, auto_stop_event_cb, LV_EVENT_CLICKED, NULL);
		
    lv_obj_t *mode_btn = create_menu_button(scr, "单动", 130, 44);
    lv_obj_align(mode_btn, LV_ALIGN_BOTTOM_LEFT, 360, -100);
    auto_mode_btn_label = lv_obj_get_child(mode_btn, 0);
    lv_obj_add_event_cb(mode_btn, auto_mode_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(mode_btn, lv_color_hex(0x66C4CB), LV_STATE_DEFAULT);
		
    bottom_nav_create(scr);
}

static void manual_page_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    reset_page_objects();
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_STATE_DEFAULT);
    time_header_create(scr, lv_color_black());

    lv_obj_t *title = create_text_label(scr, "当前页面：手动页", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    lv_obj_t *pickup_btn = create_blue_button(scr, "取料", 132, 60);
    lv_obj_align(pickup_btn, LV_ALIGN_TOP_LEFT, 88, 70);

    lv_obj_t *clamp_btn = create_blue_button(scr, "夹料", 132, 60);
    lv_obj_align(clamp_btn, LV_ALIGN_TOP_MID, 0, 70);

    lv_obj_t *rotate_btn = create_blue_button(scr, "旋转", 132, 60);
    lv_obj_align(rotate_btn, LV_ALIGN_TOP_RIGHT, -82, 70);

    lv_obj_t *x_left_btn = create_blue_button(scr, "左", 60, 52);
    lv_obj_align(x_left_btn, LV_ALIGN_TOP_LEFT, 52, 145);
    lv_obj_add_event_cb(x_left_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((0 << 8) | 0xFF));
    lv_obj_add_event_cb(x_left_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((0 << 8) | 0xFF));
    lv_obj_t *x_axis_label = create_text_label(scr, "x轴", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(x_axis_label, LV_ALIGN_TOP_LEFT, 138, 160);
    lv_obj_t *x_right_btn = create_blue_button(scr, "右", 60, 52);
    lv_obj_align(x_right_btn, LV_ALIGN_TOP_LEFT, 184, 145);
    lv_obj_add_event_cb(x_right_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((0 << 8) | 1));
    lv_obj_add_event_cb(x_right_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((0 << 8) | 1));
    x_axis_value_label = create_axis_value(scr, 110, 200, x_axis_value);

    lv_obj_t *y_forward_btn = create_blue_button(scr, "前", 60, 52);
    lv_obj_align(y_forward_btn, LV_ALIGN_TOP_MID, -88, 145);
    lv_obj_add_event_cb(y_forward_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((1 << 8) | 1));
    lv_obj_add_event_cb(y_forward_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((1 << 8) | 1));
    lv_obj_t *y_axis_label = create_text_label(scr, "y轴", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(y_axis_label, LV_ALIGN_TOP_MID, 0, 160);
    lv_obj_t *y_back_btn = create_blue_button(scr, "后", 60, 52);
    lv_obj_align(y_back_btn, LV_ALIGN_TOP_MID, 88, 145);
    lv_obj_add_event_cb(y_back_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((1 << 8) | 0xFF));
    lv_obj_add_event_cb(y_back_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((1 << 8) | 0xFF));
    y_axis_value_label = create_axis_value(scr, 360, 200, y_axis_value);

    lv_obj_t *z_up_btn = create_blue_button(scr, "上", 60, 52);
    lv_obj_align(z_up_btn, LV_ALIGN_TOP_RIGHT, -174, 145);
    lv_obj_add_event_cb(z_up_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((2 << 8) | 1));
    lv_obj_add_event_cb(z_up_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((2 << 8) | 1));
    lv_obj_t *z_axis_label = create_text_label(scr, "z轴", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(z_axis_label, LV_ALIGN_TOP_RIGHT, -126, 160);
    lv_obj_t *z_down_btn = create_blue_button(scr, "下", 60, 52);
    lv_obj_align(z_down_btn, LV_ALIGN_TOP_RIGHT, -56, 145);
    lv_obj_add_event_cb(z_down_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((2 << 8) | 0xFF));
    lv_obj_add_event_cb(z_down_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((2 << 8) | 0xFF));
    z_axis_value_label = create_axis_value(scr, 620, 200, z_axis_value);

    lv_obj_t *x_home_btn = create_blue_button(scr, "x轴回原点", 208, 48);
    lv_obj_align(x_home_btn, LV_ALIGN_TOP_LEFT, 58, 280);
    lv_obj_add_event_cb(x_home_btn, axis_home_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)0);
    lv_obj_t *z_home_btn = create_blue_button(scr, "z轴回原点", 208, 48);
    lv_obj_align(z_home_btn, LV_ALIGN_TOP_MID, 0, 275);
    lv_obj_add_event_cb(z_home_btn, axis_home_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)2);
    lv_obj_t *y_home_btn = create_blue_button(scr, "y轴回原点", 208, 48);
    lv_obj_align(y_home_btn, LV_ALIGN_TOP_LEFT, 58, 350);
    lv_obj_add_event_cb(y_home_btn, axis_home_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)1);
    lv_obj_t *z_pickup_pos_btn = create_blue_button(scr, "z轴取料位", 208, 48);
    lv_obj_align(z_pickup_pos_btn, LV_ALIGN_TOP_MID, 0, 350);

    bottom_nav_create(scr);
}

static void create_param_row(lv_obj_t *parent, const char *label_text, uint32_t *value, lv_coord_t x, lv_coord_t y)
{
    lv_obj_t *label = create_text_label(parent, label_text, &lv_font_chinese_10, lv_color_black());
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, x, y);

    lv_obj_t *textarea = create_param_textarea(parent, value);
    lv_obj_align(textarea, LV_ALIGN_TOP_LEFT, x + 80, y - 4);
}

static void parameter_page_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    reset_page_objects();
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_STATE_DEFAULT);
    time_header_create(scr, lv_color_black());

    lv_obj_t *title = create_text_label(scr, "当前页面：参数页", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    lv_obj_t *x_left_btn = create_blue_button(scr, "左", 60, 52);
    lv_obj_align(x_left_btn, LV_ALIGN_TOP_LEFT, 50, 84);
    lv_obj_add_event_cb(x_left_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((0 << 8) | 0xFF));
    lv_obj_add_event_cb(x_left_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((0 << 8) | 0xFF));
    lv_obj_t *x_axis_label = create_text_label(scr, "x轴", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(x_axis_label, LV_ALIGN_TOP_LEFT, 136, 100);
    lv_obj_t *x_right_btn = create_blue_button(scr, "右", 60, 52);
    lv_obj_align(x_right_btn, LV_ALIGN_TOP_LEFT, 180, 84);
    lv_obj_add_event_cb(x_right_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((0 << 8) | 1));
    lv_obj_add_event_cb(x_right_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((0 << 8) | 1));
    x_axis_value_label = create_axis_value(scr, 107, 136, x_axis_value);

    lv_obj_t *y_forward_btn = create_blue_button(scr, "前", 60, 52);
    lv_obj_align(y_forward_btn, LV_ALIGN_TOP_MID, -88, 84);
    lv_obj_add_event_cb(y_forward_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((1 << 8) | 1));
    lv_obj_add_event_cb(y_forward_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((1 << 8) | 1));
    lv_obj_t *y_axis_label = create_text_label(scr, "y轴", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(y_axis_label, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_t *y_back_btn = create_blue_button(scr, "后", 60, 52);
    lv_obj_align(y_back_btn, LV_ALIGN_TOP_MID, 88, 84);
    lv_obj_add_event_cb(y_back_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((1 << 8) | 0xFF));
    lv_obj_add_event_cb(y_back_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((1 << 8) | 0xFF));
    y_axis_value_label = create_axis_value(scr, 360, 136, y_axis_value);

    lv_obj_t *z_up_btn = create_blue_button(scr, "上", 60, 52);
    lv_obj_align(z_up_btn, LV_ALIGN_TOP_RIGHT, -178, 84);
    lv_obj_add_event_cb(z_up_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((2 << 8) | 1));
    lv_obj_add_event_cb(z_up_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((2 << 8) | 1));
    lv_obj_t *z_axis_label = create_text_label(scr, "z轴", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(z_axis_label, LV_ALIGN_TOP_RIGHT, -126, 100);
    lv_obj_t *z_down_btn = create_blue_button(scr, "下", 60, 52);
    lv_obj_align(z_down_btn, LV_ALIGN_TOP_RIGHT, -58, 84);
    lv_obj_add_event_cb(z_down_btn, axis_move_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((2 << 8) | 0xFF));
    lv_obj_add_event_cb(z_down_btn, axis_move_event_cb, LV_EVENT_LONG_PRESSED_REPEAT, (void *)(intptr_t)((2 << 8) | 0xFF));
    z_axis_value_label = create_axis_value(scr, 620, 136, z_axis_value);

    create_param_row(scr, "运行速度:", &x_run_speed_value, 14, 205);
    create_param_row(scr, "点动速度:", &x_jog_speed_value, 14, 250);
    create_param_row(scr, "取料位置:", &x_pickup_pos_value, 14, 295);
    create_param_row(scr, "松爪距离:", &loose_claw_distance_value, 14, 360);

    create_param_row(scr, "运行速度:", &y_run_speed_value, 300, 212);
    create_param_row(scr, "点动速度:", &y_jog_speed_value, 300, 257);
    create_param_row(scr, "取料位置:", &y_pickup_pos_value, 300, 302);

    create_param_row(scr, "运行速度:", &z_run_speed_value, 570, 212);
    create_param_row(scr, "点动速度:", &z_jog_speed_value, 570, 257);
    create_param_row(scr, "取料位置:", &z_pickup_pos_value, 570, 302);

    bottom_nav_create(scr);
}

static void position_page_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    reset_page_objects();
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_STATE_DEFAULT);
    time_header_create(scr, lv_color_black());

    lv_obj_t *title = create_text_label(scr, "当前页面：位置页", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    lv_obj_t *x_header = create_text_label(scr, "X轴", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(x_header, LV_ALIGN_TOP_LEFT, 248, 42);
    lv_obj_t *y_header = create_text_label(scr, "Y轴", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(y_header, LV_ALIGN_TOP_LEFT, 412, 42);
    lv_obj_t *z_header = create_text_label(scr, "Z轴", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(z_header, LV_ALIGN_TOP_LEFT, 578, 42);

    for(uint8_t i = 0; i < 6; i++) {
        char label_text[16];
        snprintf(label_text, sizeof(label_text), "位置%d设置:", i + 1);

        lv_coord_t y = 76 + i * 59;
        lv_obj_t *row_label = create_text_label(scr, label_text, &lv_font_chinese_16, lv_color_black());
        lv_obj_align(row_label, LV_ALIGN_TOP_LEFT, 80, y + 4);

        lv_obj_t *x_value_box = create_position_value_box(scr, &position_values[i][0]);
        lv_obj_align(x_value_box, LV_ALIGN_TOP_LEFT, 220, y);

        lv_obj_t *y_value_box = create_position_value_box(scr, &position_values[i][1]);
        lv_obj_align(y_value_box, LV_ALIGN_TOP_LEFT, 386, y);

        lv_obj_t *z_value_box = create_position_value_box(scr, &position_values[i][2]);
        lv_obj_align(z_value_box, LV_ALIGN_TOP_LEFT, 548, y);

        lv_obj_t *save_btn = create_light_button(scr, "保存", 96, 36);
        lv_obj_align(save_btn, LV_ALIGN_TOP_LEFT, 680, y);
    }

    bottom_nav_create(scr);
}

static void product_page_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);
    reset_page_objects();
    lv_obj_set_style_bg_color(scr, lv_color_white(), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_STATE_DEFAULT);
    time_header_create(scr, lv_color_black());

    lv_obj_t *title = create_text_label(scr, "当前页面：产品信息页", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 16);

    lv_obj_t *name_label = create_text_label(scr, "产品名称:", &lv_font_chinese_16, lv_color_black());
    lv_obj_align(name_label, LV_ALIGN_TOP_LEFT, 144, 214);

    lv_obj_t *name_textarea = create_product_name_textarea(scr);
    lv_obj_align(name_textarea, LV_ALIGN_TOP_LEFT, 264, 187);

    lv_obj_t *save_to_host_btn = create_blue_button(scr, "保存主机", 132, 62);
    lv_obj_align(save_to_host_btn, LV_ALIGN_TOP_RIGHT, -72, 128);
    lv_obj_add_event_cb(save_to_host_btn, param_save_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *set_to_machine_btn = create_blue_button(scr, "设置参数", 132, 62);
    lv_obj_align(set_to_machine_btn, LV_ALIGN_TOP_RIGHT, -72, 282);
    lv_obj_add_event_cb(set_to_machine_btn, param_save_event_cb, LV_EVENT_CLICKED, NULL);

    bottom_nav_create(scr);
}

/**
 * @brief  LVGL婕旂ず
 * @param  锟?
 * @return 锟?
 */
void lv_ui(void)
{
    if(time_refresh_timer == NULL) {
        time_refresh_timer = lv_timer_create(time_refresh_timer_cb, 1000, NULL);
    }

    home_page_create();
}
