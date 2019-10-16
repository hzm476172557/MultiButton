/*
 * Copyright (c) 2016 Zibin Zheng <znbin@qq.com>
 * All rights reserved
 */

#include "multi_button.h"

#define EVENT_CB(ev)        if(handle->cb[ev]) handle->cb[ev]((Button*)handle)
#define COMBINE_EVENT_CB()  if(combine_callback) combine_callback(combine_counter)

//button handle list head.
static struct Button *head_handle = NULL;

// calculate the number of combine keys now.
static uint32_t combine_counter = 0;
static int32_t combine_button_id_list[COMBINE_MAX_KEY];
// combine callback function
static BtnCallback combine_callback = NULL;

static int _button_combine_add(uint16_t button_id) {
    int result = -1;

    for(uint16_t i = 0; i < COMBINE_MAX_KEY; i++) {
        if(combine_button_id_list[i] == button_id) {
            return 0;   // already added.
        }
    }

    for(uint16_t i = 0; i < COMBINE_MAX_KEY; i++) {
        if(combine_button_id_list[i] == COMBINE_BTN_ID_NONE) {
            combine_button_id_list[i] = button_id;
            result = 0;
            break;
        }
    }

    return result;
}

static int _button_combine_del(uint16_t button_id) {
    int result = -1;

    for(uint16_t i = 0; i < COMBINE_MAX_KEY; i++) {
        if(combine_button_id_list[i] == button_id) {
            combine_button_id_list[i] = COMBINE_BTN_ID_NONE;
            result = 0;
            break;
        }
    }

    return result;
}

/**
  * @brief  Check if the specified button_id is in Combine Keys.Used when CombineEvent occurred.
  * @param  button_id: the specified button id to check.
  * @retval 0 : not in CombineKeys. others : in CombineKeys
  */
uint8_t button_combine_check(uint16_t button_id) {
    uint8_t result = 0;

    for(uint16_t i = 0; i < COMBINE_MAX_KEY; i++) {
        if(combine_button_id_list[i] == button_id) {
            result = 1;
            break;
        }
    }

    return result;
}


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
  * @brief  Attach the combine key event callback function.
  * @param  cb: callback function.
  * @retval None
  */
void button_combine_event_attach(BtnCallback cb) {
    combine_callback = cb;
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

    if(handle->event != NONE_PRESS) {
        _button_combine_add(handle->button_id);
    }

    /*-----------------State machine-------------------*/
    switch(handle->state) {
        case 0:
            if(handle->button_level == handle->active_level) {      //start press down
                combine_counter++;
                handle->event = (uint8_t)PRESS_DOWN;
                EVENT_CB(PRESS_DOWN);
                handle->ticks = 0;
                handle->repeat = 1;
                handle->state = 1;
            } else {
                if(handle->event != (uint8_t)NONE_PRESS) {
                    handle->event = (uint8_t)NONE_PRESS;
                    handle->ticks = 0;
                    combine_counter--;
                }

                _button_combine_del(handle->button_id);
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
                        EVENT_CB(DOUBLE_CLICK); // repeat hit
                    } else {
                        handle->event = (uint8_t)PRESS_REPEAT;
                        EVENT_CB(PRESS_REPEAT); // repeat hit
                    }

                    handle->state = 0;
                }
            }

            break;

        case 3:
            if(handle->button_level != handle->active_level) {
                handle->event = (uint8_t)PRESS_UP;
                EVENT_CB(PRESS_UP);

                handle->ticks = 0;
                handle->state = 2;
            } else {
                if(handle->ticks > LONG_TICKS) {
                    handle->repeat-- ;  // Detected long press, ignore the last repeat and upload the long_press event

                    if(handle->repeat == 2) {
                        handle->event = (uint8_t)DOUBLE_CLICK;
                        EVENT_CB(DOUBLE_CLICK); // repeat hit
                    } else {
                        handle->event = (uint8_t)PRESS_REPEAT;
                        EVENT_CB(PRESS_REPEAT); // repeat hit
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

    /* Check current keys. If has combine keys, it will be setted to combine mode */
}

void multi_button_init(void) {
    memset(combine_button_id_list, COMBINE_BTN_ID_NONE, COMBINE_MAX_KEY * sizeof(combine_button_id_list[0]));
    combine_counter = 0;
    combine_callback = NULL;

    head_handle = NULL;
}

