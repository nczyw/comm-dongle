#include "task_os_led.h"


rt_thread_t os_led_handle = RT_NULL;

static void task_os_led_entry(void *parameter);

/* 只做 LED GPIO 初始化，闪烁线程由 task_os_led_flash() 按需创建 */
void task_os_led_init(void) {
    os_led_init();
    task_os_led_flash(100);
}

void task_os_led_flash(rt_uint32_t duration_ms) {
    /* 上一次闪烁还没结束，忽略本次请求 */
    if (os_led_handle != RT_NULL) {
        return;
    }
    os_led_handle = rt_thread_create(
        "os_led",
        task_os_led_entry,
        (void *)(rt_ubase_t)duration_ms,   /* 闪烁总时长通过线程参数传入 */
        OS_LED_STACK_SIZE,
        OS_LED_PRIORITY,
        OS_LED_THREAD_TIMESLICE
    );
    if (os_led_handle != RT_NULL) {
        rt_thread_startup(os_led_handle);
    }
}


static void task_os_led_entry(void *parameter)
{
    rt_uint32_t duration_ms = (rt_uint32_t)(rt_ubase_t)parameter;
    rt_tick_t   start = rt_tick_get();
    rt_tick_t   timeout = rt_tick_from_millisecond(duration_ms);

    while ((rt_tick_get() - start) < timeout) {
        OS_LED();
        rt_thread_mdelay(OS_LED_BLINK_PERIOD_MS);
    }

    OS_LED_OFF();              
    os_led_handle = RT_NULL;  
}
