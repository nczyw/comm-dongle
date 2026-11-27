#ifndef TASK_OS_LED_H
#define TASK_OS_LED_H

#include "rtthread.h"

#define OS_LED_STACK_SIZE              256
#define OS_LED_PRIORITY                31
#define OS_LED_THREAD_TIMESLICE        10
#define OS_LED_BLINK_PERIOD_MS         20

#define OS_LED_EVT_CAN_RX              (1UL << 0)

extern rt_thread_t os_led_handle;
extern rt_event_t  os_led_evt;

void task_os_led_init(void);

#endif
