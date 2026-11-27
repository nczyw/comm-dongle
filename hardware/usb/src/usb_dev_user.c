// USB 用户回调：连接 / 复位 / 配置 / 挂起 / 唤醒等事件
// 本板无 VBUS 检测（未接 PA09），上电初始化时直接让设备连接

#include "usb_dev_user.h"

extern usb_core_instance usb_dev;

// =============== 函数前置声明 ===============
void usb_dev_user_init(void);
void usb_dev_user_rst(void);
void usb_dev_user_devcfg(void);
void usb_dev_user_devsusp(void);
void usb_dev_user_devresume(void);
void usb_dev_user_conn(void);
void usb_dev_user_disconn(void);

// =============== 回调表 ===============
usb_dev_user_func user_cb = {
    &usb_dev_user_init,
    &usb_dev_user_rst,
    &usb_dev_user_devcfg,
    &usb_dev_user_devsusp,
    &usb_dev_user_devresume,
    &usb_dev_user_conn,
    &usb_dev_user_disconn,
};

// =============== 函数实现 ===============

// 初始化：无 VBUS 检测，直接连接（D+ 上拉）
void usb_dev_user_init(void)
{
    usb_ctrldevconnect(&usb_dev.regs, 0);   // 0 = 连接，1 = 断开
}

// USB 复位完成
void usb_dev_user_rst(void)
{
}

// 配置完成（枚举成功，进入配置态）
void usb_dev_user_devcfg(void)
{
}

// 进入挂起
void usb_dev_user_devsusp(void)
{
}

// 从挂起唤醒
void usb_dev_user_devresume(void)
{
}

// 设备连接事件
void usb_dev_user_conn(void)
{
}

// 设备断开事件
void usb_dev_user_disconn(void)
{
}
