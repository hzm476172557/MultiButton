/*
 * Copyright (c) 2016 Zibin Zheng <znbin@qq.com>
 * All rights reserved
 */

#ifndef _MULTI_BUTTON_H_
#define _MULTI_BUTTON_H_

#include "stdint.h"
#include "string.h"
/* For user configure ******************************************/
// �����Ƿ����ü��REPEAT(����DoubleClick)���ܣ��������REPEAT���ܣ�����ÿ�ε������Ҫ��ʱREPEAT_TICKSʱ�����´�������
#define REPEAT_MODE_ENABLE      (1)

// �����Ƿ�������ϰ������ܣ����������ϰ���������ÿ�μ�ⰴ�������ڣ�����Ĵ������࣬���ܵ����жϴ������
// ������������ɨ����BTN_TICKS_INTERVAL 10ms.
// ����COMBINEģʽ�󣬵�1�����ϰ�������(������ϼ�ģʽ)����ȥPRESS_DOWN��PRESS_UP�������ٴ������������������¼���ֱ�����а������ɿ����ָ����а���Ϊ����״̬��
#define COMBINE_MODE_ENABLE     (1)

#define COMBINE_MAX_KEY         3                               // ���������ϼ������������������ƺ��µİ�����������
#define COMBINE_BTN_ID_NONE     (-1)                            // ��ϰ�������Ч������ʾ

//According to your need to modify the constants.
#define BTN_TICKS_INTERVAL      10 //ms                         // ��ʱ���ڣ�����޸��ˣ���ʱɨ�躯����button_ticks��������ɨ��ʱ����Ҫͬ������
#define DEBOUNCE_TICKS          3 //MAX 64                      // ȥ��ʱ��
#define SHORT_TICKS             (50     / BTN_TICKS_INTERVAL)   // �����̰���ʱ��
#define REPEAT_TICKS            (100    / BTN_TICKS_INTERVAL)   // ���������޶�ʱ�䡣�ڴ���SHORT�������ڸ��޶��¼����ٴΰ���������뷴������ģʽ����ʱδ�ٰ��������ж�Ϊ�ɿ�
#define LONG_TICKS              (1000   / BTN_TICKS_INTERVAL)   // ����������ʱ��
#define LONG_WAIT_TICKS         (500    / BTN_TICKS_INTERVAL)   // ���������󣬵ȴ���ʱ��󣬲ſ�ʼ���HOLD�����ⳤ���������̴���HOLD
#define HOLD_PERIOD_TICKS       (200    / BTN_TICKS_INTERVAL)   // ���ֳ���ʱ��ÿ���ϱ����ڳ���״̬�ļ��ʱ��
#define COMBINE_TICKS           (5000   / BTN_TICKS_INTERVAL)   // ������ϰ����󣬰��������¼��Ż��ϱ���ϰ����¼�

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
<<<<<<< HEAD
#endif
=======

>>>>>>> 885d2d0f96a1c82cb40d41e5b2dff303f5168870

#ifdef __cplusplus
}
#endif

#endif
