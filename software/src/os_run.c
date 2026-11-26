#include "i2c.h"
#include "can.h"
#include "os_run.h"

void os_run(void){
    // 必须操作系统启动后, 才能初始化, 因为使用了操作系统相关的东西
    i2c1_init();
    can_init();
    task_os_led_init();
}
