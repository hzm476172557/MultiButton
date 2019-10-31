/*
 * Copyright (c) 2016 Zibin Zheng <znbin@qq.com>
 * All rights reserved
 */

#include "multi_button.h"

#define EVENT_CB(ev)        if(handle->cb[ev]) handle->cb[ev]((Button*)handle)
//button handle list head.
static struct Button *head_handle = NULL;

#if (COMBINE_MODE_ENABLE > 0)

// Callback function to upload combine event to user. User can use button_combine_check() to check combine buttons id.
#define COMBINE_EVENT_CB(param)  if(btnComb.cb) btnComb.cb(param)

typedef struct _ButtonCombine {
    // combine pressing buttons counter
    // record the number of current pressing buttons.
    uint16_t    counter;

    // when detected combine buttons, ticks start to count the time from the beginning.
    // when ticks count greater than COMBINE_LONG_TICKS define, COMBINE_EVENT_CB will be call to report to app.
    uint16_t    ticks;

    // combine buttons invalid flag.
    // Combine invalid if number of current pressing buttons greater than COMBINE_MAX_KEY define.
    // 0 means current combine buttons is valid.
    // 1 means current combine buttons is invalid.
    uint8_t     isInvalid : 1;

    // combine buttons change flag.
    // 0 means none change since last combine pressing.
    // 1 means detected the pressing buttons changes.Neet to reset ticks.
    uint8_t     isChanged : 1;
    // Combine Mode :
    //   0 means none combine button
    //   1 means detected combine buttons
    //   2 means tick counter finished and callback func has been called
    //   3 means invalid combine buttons detected, do nothing then.
    uint8_t     mode : 5;

    // Combine short_press detect flag :
    //   0 means none short press
    //   1 means detected short combine press
    uint8_t     short_press : 1;

    // combine buttons pressing list to stores pressing combine buttons when short pressed.
    int32_t btnCombListForShortPress[COMBINE_MAX_KEY];

    // combine buttons pressing list.stores current pressing combine buttons.
    // each element value is the each id of the pressing buttons. COMBINE_BTN_ID_NONE means no buttons in this element.
    // COMBINE_MAX_KEY defines the maximum combine buttons allowed at a time.
    int32_t btnCombList[COMBINE_MAX_KEY];



    // combine event callback function
    BtnCallback cb;
} __attribute__((packed)) BtnCombT;

static volatile BtnCombT btnComb;

static uint8_t _button_combine_add(uint16_t button_id) {
    int result = 1;

    for(uint16_t i = 0; i < COMBINE_MAX_KEY; i++) {
        if(btnComb.btnCombList[i] == button_id) {
            return 0;   // already added.
        }
    }

    for(uint16_t i = 0; i < COMBINE_MAX_KEY; i++) {
        if(btnComb.btnCombList[i] == COMBINE_BTN_ID_NONE) {
            btnComb.btnCombList[i] = button_id;

            btnComb.isChanged = 1;
            return 0;
        }
    }

    return result;
}

static uint8_t _button_combine_del(uint16_t button_id) {
    for(uint16_t i = 0; i < COMBINE_MAX_KEY; i++) {
        if(btnComb.btnCombList[i] == button_id) {
            btnComb.btnCombList[i] = COMBINE_BTN_ID_NONE;

            btnComb.isChanged = 1;
            break;
        }
    }

    return 0;
}

static void _button_set_combine_state(void) {
    struct Button *target;

    btnComb.mode = 1;

    for(target = head_handle; target; target = target->next) {
        if(target->event != (uint8_t)NONE_PRESS && target->event != (uint8_t)PRESS_UP) {
            target->event = (uint8_t)PRESS_DOWN;    // 其它按键只要是处于按下状态的，全部设置为PRESS_DOWN
        } else {
            target->event = (uint8_t)NONE_PRESS;
        }

        target->debounce_cnt = 0;
        target->ticks = 0;
        target->state = 7;  // 所有按键进入combine 状态(state 7)
    }
}

static void _button_reset_combine_state(void) {
    struct Button *target;

    btnComb.mode = 0;

    for(target = head_handle; target; target = target->next) {
        target->event = NONE_PRESS;
        target->debounce_cnt = 0;
        target->ticks = 0;
        target->state = 0;  // 所有按键恢复到state 0状态
    }
}

void button_combine_handler(void) {
    if(btnComb.mode > 0) {
        if(btnComb.counter == 0) {
            // 检测到所有按键都已经松开
            // 检测是否达到组合键短按条件
            if(btnComb.short_press != 0) {
                if(btnComb.mode < 2) {
                    // 尚未触发组合按键的长按，上报上层告知出现短按
                    memcpy((void *)btnComb.btnCombList, (void *)btnComb.btnCombListForShortPress, sizeof(btnComb.btnCombList));
                    COMBINE_EVENT_CB((void *)COMBINE_SHORT);
                }
            }

            // 将按键松开
            btnComb.ticks = 0;
            btnComb.isInvalid = 0;
            btnComb.isChanged = 0;
            btnComb.short_press = 0;
            memset((void *)btnComb.btnCombListForShortPress, COMBINE_BTN_ID_NONE, COMBINE_MAX_KEY * sizeof(btnComb.btnCombListForShortPress[0]));
            _button_reset_combine_state();
        } else {
            if(btnComb.mode == 1 && btnComb.counter > 1) {
                // not finish combine function yet.
                if(btnComb.isChanged == 0) {
                    btnComb.ticks++;

                    if(btnComb.ticks > COMBINE_SHORT_TICKS) {
                        // 到达组合按键短按的识别，将当前按键组合记录到短按内
                        btnComb.short_press = 1;
                        memcpy((void *)btnComb.btnCombListForShortPress, (void *)btnComb.btnCombList, sizeof(btnComb.btnCombListForShortPress));
                    }

                    if(btnComb.ticks > COMBINE_LONG_TICKS) {
                        if(btnComb.isInvalid == 0) {
                            btnComb.short_press = 0;
                            // finish combine function, call the COMBINE_EVENT_CB to report to app.
                            COMBINE_EVENT_CB((void *)COMBINE_LONG);
                            // after report, do nothing untill all buttons released.
                            btnComb.mode  = 2;
                        }
                    }
                } else {
                    // detected pressing buttons changed, reset the ticks.
                    btnComb.ticks     = 0;
                    btnComb.isChanged = 0;
                    memset((void *)btnComb.btnCombListForShortPress, COMBINE_BTN_ID_NONE, COMBINE_MAX_KEY * sizeof(btnComb.btnCombListForShortPress[0]));
                }

                if(btnComb.isInvalid != 0) {
#if (COMBINE_INVALID_MODE == 1)
                    // InValid Combine Buttons detected, ignore this combine event and wait untill all buttons released.
                    btnComb.mode  = 3;
#endif
                }
            } else if(btnComb.mode == 1 && btnComb.counter == 1) {
                // 处于mode = 1，但是检测到当前只有一个按键按着
                if(btnComb.isChanged == 0) {

                    btnComb.ticks++;

                    if(btnComb.ticks > COMBINE_INVALID_TICKS) {
#if (COMBINE_INVALID_MODE == 0)   // 是否需要设置为这种情况超时后，退出组合键模式
                        // 超时，退出组合按键模式
                        btnComb.ticks = 0;
                        btnComb.isInvalid = 0;
                        btnComb.isChanged = 0;
                        btnComb.counter = 0;
                        _button_reset_combine_state();
#else   // 如果不退出组合键模式，则超时后进入无效组合键模式，直到松开所有按键重新触发按键
                        btnComb.ticks = 0;
                        btnComb.isInvalid = 1;
                        btnComb.mode  = 3;
#endif
                    }

                } else {
                    btnComb.ticks = 0;
                    btnComb.isChanged = 0;
                }
            }
        }
    }
}

/**
  * @brief  Check if the specified button_id is in Combine Keys.Used when CombineEvent occurred.
  *         User can use this to check if button_id in the CombineBottonList or not in the callback func.
  * @param  button_id: the specified button id to check.
  * @retval 0 : not in CombineKeys. others : in CombineKeys
  */
uint8_t button_combine_check(uint16_t button_id) {
    uint8_t result = 0;

    for(uint16_t i = 0; i < COMBINE_MAX_KEY; i++) {
        if(btnComb.btnCombList[i] == button_id) {
            result = 1;
            break;
        }
    }

    return result;
}

/**
  * @brief  Attach the combine key event callback function.
  * @param  cb: callback function.
  * @retval None
  */
void button_combine_event_attach(BtnCallback cb) {
    btnComb.cb = cb;
}

/**
  * @brief  Detach the combine key event callback function.
  * @retval None
  */
void button_combine_event_detach(void) {
    btnComb.cb = NULL;
}


#endif

/**
  * @brief  Initializes the button struct handle.
  * @param  handle: the button handle strcut.
  * @param  pin_level: read the HAL GPIO of the connet button level.
  * @param  active_level: pressed GPIO level.
  * @param  button_id:The unique id for this button
  * @retval None
  */
void button_init(struct Button *handle, uint8_t(*pin_level)(void *btn), uint8_t active_level, uint16_t button_id) {
    memset(handle, 0, sizeof(struct Button));
    handle->button_id = button_id;
    handle->event = (uint8_t)NONE_PRESS;
    handle->hal_button_Level = pin_level;
    handle->button_level = handle->hal_button_Level((void *)handle);
    handle->active_level = active_level;
}

/**
  * @brief  Attach the button event callback function.
  * @param  handle: the button handle strcut.
  * @param  event: trigger event type.
  * @param  cb: callback function.
  * @retval None
  */
void button_attach(struct Button *handle, PressEvent event, BtnCallback cb) {
    if(event < number_of_event) {
        handle->cb[event] = cb;
    }
}

/**
  * @brief  Inquire the button event happen.
  * @param  handle: the button handle strcut.
  * @retval button event.
  */
PressEvent get_button_event(struct Button *handle) {
    return (PressEvent)(handle->event);
}

uint8_t get_button_repeat_count(struct Button *handle) {
    return handle->repeat;
}

/**
  * @brief  Button driver core function, driver state machine.
  * @param  handle: the button handle strcut.
  * @retval None
  */
void button_handler(struct Button *handle) {
    uint8_t read_gpio_level = handle->hal_button_Level((void *)handle);

    //ticks counter working..
    if((handle->state) > 0) {
        handle->ticks++;
    }

    /*------------button debounce handle---------------*/
    if(read_gpio_level != handle->button_level) { //not equal to prev one
        //continue read 3 times same new level change
        if(++(handle->debounce_cnt) >= DEBOUNCE_TICKS) {
            handle->button_level = read_gpio_level;
            handle->debounce_cnt = 0;
        }
    } else { //leved not change ,counter reset.
        handle->debounce_cnt = 0;
    }

#if (COMBINE_MODE_ENABLE > 0)

    if(handle->event != NONE_PRESS) {
        if(btnComb.isInvalid == 0) {
            btnComb.isInvalid = _button_combine_add(handle->button_id);
        }
    }

#endif

    /*-----------------State machine-------------------*/
    switch(handle->state) {
        case 0:
            if(handle->button_level == handle->active_level) {      //start press down
#if (COMBINE_MODE_ENABLE > 0)
                btnComb.counter++;
#endif
                handle->event = (uint8_t)PRESS_DOWN;
                EVENT_CB(PRESS_DOWN);
                handle->ticks = 0;
                handle->repeat = 1;
#if (COMBINE_MODE_ENABLE > 0)

                if(btnComb.counter > 1) {
                    // 检测到有组合按键，设置所有按键进入组合按键模式
                    _button_set_combine_state();
                } else {
                    handle->state = 1;
                }

#else
                handle->state = 1;
#endif
            } else {
                if(handle->event != (uint8_t)NONE_PRESS) {
                    handle->ticks = 0;
                    handle->event = (uint8_t)NONE_PRESS;
#if (COMBINE_MODE_ENABLE > 0)
                    btnComb.counter--;
#endif
                }

#if (COMBINE_MODE_ENABLE > 0)
                btnComb.isInvalid = _button_combine_del(handle->button_id);
#endif
            }

            break;

        case 7:
            if(handle->button_level == handle->active_level) {
                if(handle->event != (uint8_t)PRESS_DOWN) {
#if (COMBINE_MODE_ENABLE > 0)
                    btnComb.counter++;
#endif
                    handle->event = (uint8_t)PRESS_DOWN;
                    EVENT_CB(PRESS_DOWN);
                    handle->ticks  = 0;
                    handle->repeat = 1;
                }
            } else {
                if(handle->event != (uint8_t)NONE_PRESS) {
                    handle->event = (uint8_t)PRESS_UP;
                    EVENT_CB(PRESS_UP);
                    handle->event = (uint8_t)NONE_PRESS;

                    handle->ticks = 0;
#if (COMBINE_MODE_ENABLE > 0)
                    btnComb.counter--;
                    btnComb.isInvalid = _button_combine_del(handle->button_id);
#endif
                }
            }

            break;

        case 1:
            if(handle->button_level != handle->active_level) {      //released press up
                handle->event = (uint8_t)PRESS_UP;
                EVENT_CB(PRESS_UP);

                if(handle->ticks >= SHORT_TICKS) {
                    // SHORT_TICKS has been occurred, then state 2 to wait REPEAT_TICKS
                    handle->event = (uint8_t)SINGLE_CLICK;
                }

                handle->ticks = 0;
#if (REPEAT_MODE_ENABLE != 0)
                // If REPEAT_MODE is enabled, switch to state 2 detecting the repeat click.
                handle->state = 2;
#else

                // If REPEAT_MODE is disabled, return to state 0 after press_up
                if(handle->event == (uint8_t)SINGLE_CLICK) {
                    EVENT_CB(SINGLE_CLICK);
                }

                handle->state = 0;
#endif
            } else if(handle->ticks > LONG_TICKS) {
                handle->event = (uint8_t)LONG_RRESS_START;
                EVENT_CB(LONG_RRESS_START);
                handle->ticks = 0;
                handle->state = 6;
            }

            break;

        case 2:
            if(handle->button_level == handle->active_level) { //press down again
                handle->event = (uint8_t)PRESS_DOWN;
                EVENT_CB(PRESS_DOWN);
                handle->repeat++;
                handle->ticks = 0;
                handle->state = 3;
            } else {
                if(handle->ticks > REPEAT_TICKS) { //released timeout, back to state 0
                    if(handle->repeat == 1) {
                        if(handle->event == SINGLE_CLICK) {
                            EVENT_CB(SINGLE_CLICK);
                        }
                    } else if(handle->repeat == 2) {
                        handle->event = (uint8_t)DOUBLE_CLICK;
                        EVENT_CB(DOUBLE_CLICK);    // repeat hit
                    } else {
                        //handle->event = (uint8_t)PRESS_REPEAT;
                        EVENT_CB(PRESS_REPEAT);    // repeat hit
                    }

                    handle->state = 0;
                }
            }

            break;

        case 3:
            if(handle->button_level != handle->active_level) {
                handle->event = (uint8_t)PRESS_UP;
                EVENT_CB(PRESS_UP);

                handle->event = NONE_PRESS;
                handle->ticks = 0;
                handle->state = 2;
            } else {
                if(handle->ticks > LONG_TICKS) {
                    handle->repeat-- ;  // Detected long press, ignore the last repeat and upload the long_press event

                    if(handle->repeat == 2) {
                        handle->event = (uint8_t)DOUBLE_CLICK;
                        EVENT_CB(DOUBLE_CLICK);    // repeat hit
                    } else {
                        //handle->event = (uint8_t)PRESS_REPEAT;
                        EVENT_CB(PRESS_REPEAT);    // repeat hit
                    }

                    handle->event = (uint8_t)LONG_RRESS_START;
                    EVENT_CB(LONG_RRESS_START);
                    handle->ticks = 0;
                    handle->state = 6;
                }
            }

            break;

        case 5:
            if(handle->button_level == handle->active_level) {
                //continue hold trigger
                if(handle->ticks > HOLD_PERIOD_TICKS) {
                    handle->event = (uint8_t)LONG_PRESS_HOLD;
                    EVENT_CB(LONG_PRESS_HOLD);
                    handle->ticks = 0;  // clean the tick for next HOLD event.
                }

            } else { //releasd
                handle->event = (uint8_t)PRESS_UP;
                EVENT_CB(PRESS_UP);
                handle->state = 0; //reset
            }

            break;

        case 6:
            if(handle->button_level == handle->active_level) {
                // if user still pressing the button over LONG_WAIT_TICKS, go to state 5 to detect HOLD event.
                if(handle->ticks > LONG_WAIT_TICKS) {
                    handle->event = (uint8_t)LONG_PRESS_HOLD;
                    EVENT_CB(LONG_PRESS_HOLD);
                    handle->ticks = 0;
                    handle->state = 5;
                }

            } else {
                // if user release the button before reaching LONG_WAIT_TICKS, go back to state 0 for idle
                // 未到达LONG_WAIT_TICKS就释放按键，回到空闲状态
                handle->event = (uint8_t)PRESS_UP;
                EVENT_CB(PRESS_UP);
                handle->state = 0; //reset
            }

            break;
    }
}

/**
  * @brief  Start the button work, add the handle into work list.
  * @param  handle: target handle strcut.
  * @retval 0: succeed. -1: already exist.
  */
int button_start(struct Button *handle) {
    struct Button *target = head_handle;

    while(target) {
        if(target == handle) {
            return -1;    //already exist.
        }

        target = target->next;
    }

    handle->next = head_handle;
    head_handle = handle;
    return 0;
}

/**
  * @brief  Stop the button work, remove the handle off work list.
  * @param  handle: target handle strcut.
  * @retval None
  */
void button_stop(struct Button *handle) {
    struct Button **curr;

    for(curr = &head_handle; *curr;) {
        struct Button *entry = *curr;

        if(entry == handle) {
            *curr = entry->next;
//          free(entry);
        } else {
            curr = &entry->next;
        }
    }
}

/**
  * @brief  background ticks, timer repeat invoking interval 5ms.
  * @param  None.
  * @retval None
  */
void button_ticks() {
    struct Button *target;

    for(target = head_handle; target; target = target->next) {
        button_handler(target);
    }

#if (COMBINE_MODE_ENABLE > 0)
    /* Check current keys. If has combine keys, it will be setted to combine mode */
    button_combine_handler();
#endif
}

void multi_button_init(void) {
#if (COMBINE_MODE_ENABLE > 0)
    memset((void *)&btnComb, 0, sizeof(btnComb));
    memset((void *)btnComb.btnCombList, COMBINE_BTN_ID_NONE, COMBINE_MAX_KEY * sizeof(btnComb.btnCombList[0]));
    memset((void *)btnComb.btnCombListForShortPress, COMBINE_BTN_ID_NONE, COMBINE_MAX_KEY * sizeof(btnComb.btnCombListForShortPress[0]));
    btnComb.cb = NULL;
#endif
    head_handle = NULL;
}

