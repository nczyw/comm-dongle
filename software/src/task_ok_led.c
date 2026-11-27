#include "task_os_led.h"
#include "mygpio.h"

rt_thread_t os_led_handle = RT_NULL;
rt_event_t  os_led_evt    = RT_NULL;

static void task_os_led_entry(void *parameter);

void task_os_led_init(void) {
    os_led_init();
    os_led_evt = rt_event_create("os_led_evt", RT_IPC_FLAG_FIFO);
    if (os_led_evt == RT_NULL) {
        OS_LED_ON();
        while(1);
    }
    os_led_handle = rt_thread_create(
        "os_led",
        task_os_led_entry,
        RT_NULL,
        OS_LED_STACK_SIZE,
        OS_LED_PRIORITY,
        OS_LED_THREAD_TIMESLICE
    );
    if (os_led_handle != RT_NULL) {
        rt_thread_startup(os_led_handle);
    } else {
        OS_LED_ON();
        while(1);
    }
    rt_event_send(os_led_evt, OS_LED_EVT_CAN_RX);   // 亮一下灯
}

static void task_os_led_entry(void *parameter) {
    rt_uint32_t recved;
    RT_UNUSED(parameter);
    while(1) {
        if (rt_event_recv(os_led_evt, OS_LED_EVT_CAN_RX,
                          RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                          RT_WAITING_FOREVER, &recved) == RT_EOK
        ) {
            OS_LED_ON();
            rt_thread_mdelay(OS_LED_BLINK_PERIOD_MS);
            OS_LED_OFF();
            rt_thread_mdelay(OS_LED_BLINK_PERIOD_MS);
        }
    }
}
