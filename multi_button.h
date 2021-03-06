/*
 * Copyright (c) 2016 Zibin Zheng <znbin@qq.com>
 * All rights reserved
 */

#ifndef _MULTI_BUTTON_H_
#define _MULTI_BUTTON_H_

#include "stdint.h"
#include "string.h"
/* For user configure ******************************************/
// 配置是否启用检测REPEAT(包括DoubleClick)功能，如果启用REPEAT功能，则在每次点击后，需要延时REPEAT_TICKS时间检测下次连击。
// 因此为了按键的灵敏性，建议如果应用不用检测DoubleClick或者连击功能，则不启用该模式
#define REPEAT_MODE_ENABLE      (0)

// 配置是否启用组合按键功能，如果启用组合按键，则在每次检测按键周期内，处理的代码增多，可能导致中断处理过长
// 建议增长按键扫描间隔BTN_TICKS_INTERVAL 10ms.
// 启用COMBINE模式后，当1个以上按键按下(进入组合键模式)，除去PRESS_DOWN和PRESS_UP，不会再触发单个按键的其它事件。直到所有按键都松开，恢复所有按键为空闲状态。
#define COMBINE_MODE_ENABLE     (1)

#define COMBINE_MAX_KEY         2                               // 最多允许组合键个数，按键超过限制后，新的按键将被无视
#define COMBINE_BTN_ID_NONE     (-1)                            // 组合按键，无效按键表示

// 配置是否启用无效按键模式
// 如果启用，则当检测到无效按键时(例如组合按键超过COMBINE_MAX_KEY限定个数， 或者出现组合按键后，又保持只有一个按键)，进入无效模式，不会继续处理按键直到所有按键全部松开
// 如果不启用，则忽视上述的无效按键事件，取最终的按键结果。例如超过限定个数的组合键，则取最后按下的两个按键，如果只保持一个按键，则直接退出组合按键，按单个键进行重新检测
#define COMBINE_INVALID_MODE    (1)

//According to your need to modify the constants.
#define BTN_TICKS_INTERVAL      10 //ms                         // 定时周期，如果修改了，则定时扫描函数（button_ticks）的周期扫描时间需要同步调整
#define DEBOUNCE_TICKS          3 //MAX 64                      // 去抖时间
#define SHORT_TICKS             (50     / BTN_TICKS_INTERVAL)   // 触发短按的时间
#define REPEAT_TICKS            (50     / BTN_TICKS_INTERVAL)   // 反复按键限定时间。在触发SHORT按键后，在该限定事件内再次按键，则进入反复按键模式。超时未再按键，则判定为松开
#define LONG_TICKS              (2000   / BTN_TICKS_INTERVAL)   // 触发长按的时间
#define LONG_WAIT_TICKS         (500    / BTN_TICKS_INTERVAL)   // 触发长按后，等待该时间后，才开始检测HOLD，避免长按结束立刻触发HOLD
#define HOLD_PERIOD_TICKS       (300    / BTN_TICKS_INTERVAL)   // 保持长按时，每次上报处于长按状态的间隔时间
#define COMBINE_INVALID_TICKS   (500    / BTN_TICKS_INTERVAL)   // 出现组合按键后，后续又不符合组合按键要求，例如又变成单键，持续超时后，退出组合按键模式
#define COMBINE_SHORT_TICKS     (50     / BTN_TICKS_INTERVAL)   // 出现组合按键后，当前组合键达到该时间后，这时松开手，则视为触发短按组合按键
#define COMBINE_LONG_TICKS      (5000   / BTN_TICKS_INTERVAL)   // 出现组合按键后，按超过该事件后，就会上报组合按键事件(不管松不松手)

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
    COMBINE_SHORT,
    COMBINE_LONG,
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

#if (COMBINE_MODE_ENABLE > 0)
uint8_t button_combine_check(uint16_t button_id);
void button_combine_event_attach(BtnCallback cb);
void button_combine_event_detach(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
