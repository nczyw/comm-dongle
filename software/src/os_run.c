#include "i2c.h"
#include "task_can.h"
#include "os_run.h"
//#include "usb_app_conf.h"
#include "usb_dev_core.h"
#include "usb_dev_desc.h"
#include "usb_dev_user.h"
#include "usb_dev_winusb_class.h"

// USB 设备实例（usb_bsp.c / usb_dev_user.c 里以 extern 引用）
usb_core_instance usb_dev;

// USB 初始化：WinUSB 自定义类
// 说明：usb_dev_init 内部会自动调用 usb_bsp_init() 和 usb_bsp_nvicconfig()
static void usb_app_init(void)
{
    stc_usb_port_identify stcPortIdentify;
    stcPortIdentify.u8CoreID = USBFS_CORE_ID;
    usb_desc_cfg_load(); // 先读 EEPROM 里的描述符字符串到 RAM（USB 枚举前，getter 只读 RAM）
    usb_dev_init(&usb_dev, &stcPortIdentify, &user_desc, &class_winusb_cbk, &user_cb);
}

void os_run(void){
    // 必须操作系统启动后, 才能初始化, 因为使用了操作系统相关的东西
    // USB 优先初始化, 避免被前面的 EEPROM(I2C) 读取等耗时操作拖延枚举
    
    i2c1_init();
       
    usb_app_init();
    task_os_led_init();
    task_can_init();
}
