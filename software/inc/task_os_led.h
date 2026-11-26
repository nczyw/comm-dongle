#ifndef TASK_OS_LED_H
#define TASK_OS_LED_H

#include "mygpio.h"
#include "rtthread.h"

#define OS_LED_STACK_SIZE              256
#define OS_LED_PRIORITY                31
#define OS_LED_THREAD_TIMESLICE        10
#define OS_LED_BLINK_PERIOD_MS         20

extern rt_thread_t os_led_handle;
void task_os_led_init(void);
/* 开始闪烁：每 OS_LED_BLINK_PERIOD_MS 翻转一次，持续 duration_ms 后熄灭并结束任务 */
void task_os_led_flash(rt_uint32_t duration_ms);

#endif