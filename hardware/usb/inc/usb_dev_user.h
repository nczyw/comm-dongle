#ifndef __USB_DEV_USER_H__
#define __USB_DEV_USER_H__

// C++ 编译时按 C 方式链接
#ifdef __cplusplus
extern "C"
{
#endif

#include "usb_dev_core.h"

// 用户回调表（定义在 usb_dev_user.c）
extern usb_dev_user_func user_cb;

#ifdef __cplusplus
}
#endif

#endif // __USB_DEV_USER_H__
