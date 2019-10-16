/*
 * Copyright (c) 2016 Zibin Zheng <znbin@qq.com>
 * All rights reserved
 */

#ifndef _MULTI_BUTTON_H_
#define _MULTI_BUTTON_H_

#include "stdint.h"
#include "string.h"
/* For user configure ******************************************/
#define REPEAT_MODE_ENABLE      (0)                             // 配置是否启用检测REPEAT功能，如果启用REPEAT功能，则在每次点击后，需要延时REPEAT_TICKS时间检测下次连击。

#define COMBINE_MAX_KEY         3                               // 最多允许组合键个数，按键超过限制后，新的按键将被无视
#define COMBINE_BTN_ID_NONE     (~(0U))                         // 组合按键，无效按键表示

//According to your need to modify the constants.
#define BTN_TICKS_INTERVAL      5 //ms                          // 定时周期，如果修改了，则定时扫描函数（button_ticks）的周期扫描时间需要同步调整
#define DEBOUNCE_TICKS          3 //MAX 64                      // 去抖时间
#define SHORT_TICKS             (50     / BTN_TICKS_INTERVAL)   // 触发短按的时间
#define REPEAT_TICKS            (100    / BTN_TICKS_INTERVAL)   // 反复按键限定时间。在触发SHORT按键后，在该限定事件内再次按键，则进入反复按键模式。超时未再按键，则判定为松开
#define LONG_TICKS              (1000   / BTN_TICKS_INTERVAL)   // 触发长按的时间
#define LONG_WAIT_TICKS         (500    / BTN_TICKS_INTERVAL)   // 触发长按后，等待该时间后，才开始检测HOLD，避免长按结束立刻触发HOLD
#define HOLD_PERIOD_TICKS       (200    / BTN_TICKS_INTERVAL)   // 保持长按时，每次上报处于长按状态的间隔时间
#define COMBINE_TICKS           (5000   / BTN_TICKS_INTERVAL)   // 出现组合按键后，按超过该事件才会上报组合按键事件

typedef void (*BtnCallback)(void *);

typedef enum {
    PRESS_DOWN = 0,
    PRESS_UP,
    PRESS_REPEAT,
    SINGLE_CLICK,
    DOUBLE_CLICK,
    LONG_RRESS_START,
    LONG_PRESS_HOLD,
    number_of_event,
    COMBINE_PRESS,
    NONE_PRESS
} PressEvent;

typedef struct Button {
    uint16_t    button_id;
    uint16_t    ticks;
    uint8_t     repeat;
    uint8_t     event : 4;
    uint8_t     state : 4;
    uint8_t     debounce_cnt : 6;
    uint8_t     active_level : 1;
    uint8_t     button_level : 1;
    uint8_t (*hal_button_Level)(void *btn);
    BtnCallback  cb[number_of_event];
    struct Button *next;
} Button;

#ifdef __cplusplus
extern "C" {
#endif

void multi_button_init(void);

void button_init(struct Button *handle, uint8_t(*pin_level)(void *btn), uint8_t active_level, uint16_t button_id);
void button_attach(struct Button *handle, PressEvent event, BtnCallback cb);
PressEvent get_button_event(struct Button *handle);
uint8_t get_button_repeat_count(struct Button *handle);
int  button_start(struct Button *handle);
void button_stop(struct Button *handle);
void button_ticks(void);

uint8_t button_combine_check(uint16_t button_id);
void button_combine_set_callback(BtnCallback cb);

#ifdef __cplusplus
}
#endif

#endif
