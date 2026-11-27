#ifndef __USB_BSP_H__
#define __USB_BSP_H__

// C++ 编译时按 C 方式链接
#ifdef __cplusplus
extern "C"
{
#endif

#include "usb_lib.h"
#include "hc32_ll.h"

// USB BSP 初始化（引脚 + 时钟源 + 外设时钟）
void usb_bsp_init(usb_core_instance *pdev, stc_usb_port_identify *pstcPortIdentify);
// USB 中断 NVIC 配置
void usb_bsp_nvicconfig(usb_core_instance *pdev);
// 微秒 / 毫秒延时
void usb_udelay(const uint32_t usec);
void usb_mdelay(const uint32_t msec);

#ifdef __cplusplus
}
#endif

#endif // __USB_BSP_H__
