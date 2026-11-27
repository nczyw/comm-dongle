#ifndef __USB_DEV_DESC_H__
#define __USB_DEV_DESC_H__

// C++ 编译时按 C 方式链接
#ifdef __cplusplus
extern "C"
{
#endif

#include "usb_dev_def.h"

// =============== 描述符类型 ===============
#define USB_DEVICE_DESCRIPTOR_TYPE              (0x01)
#define USB_CFG_DESCRIPTOR_TYPE                 (0x02)
#define USB_STRING_DESCRIPTOR_TYPE              (0x03)
#define USB_INTERFACE_DESCRIPTOR_TYPE           (0x04)
#define USB_ENDPOINT_DESCRIPTOR_TYPE            (0x05)
#define USB_SIZ_DEVICE_DESC                     (18)
#define USB_SIZ_STRING_LANGID                   (4)

// 描述符回调表（定义在 usb_dev_desc.c）
extern usb_dev_desc_func user_desc;

#ifdef __cplusplus
}
#endif

#endif // __USB_DEV_DESC_H__
