// USB 设备描述符 + 字符串描述符（WinUSB 自定义类）
// 说明：本文件额外提供 MS OS(0xEE) 字符串描述符，配合 usb_dev_winusb_class，
//       让 Windows 枚举时自动绑定 WinUSB 驱动（免驱）

#include "usb_dev_desc.h"
#include "usb_dev_stdreq.h"

// =============== 设备 VID / PID（可改成你自己的） ===============
// 注意：不要和已有设备冲突，PID 用了 0x4604，和官方 CDC 例程的 0x4603 区分开
#define DEV_VID                        (0x2E88)
#define DEV_PID                        (0x4604)

// =============== 字符串描述符内容 ===============
#define DEV_LANGID_STRING              (0x409)              // 英语（美国）
#define DEV_MANUFACTURER_STRING        ("Rayyi-tech")
#define DEV_PRODUCT_FS_STRING          ("Rayyi MultiLink USB Interface Box")
#define DEV_SERIALNUMBER_FS_STRING     ("00000000050C")
#define DEV_CONFIGURATION_FS_STRING    ("MultiLink USB Interface Box")
#define DEV_INTERFACE_FS_STRING        ("MultiLink USB Interface Box Interface")

// =============== 函数前置声明 ===============
uint8_t *usb_dev_desc(uint16_t *length);
uint8_t *usb_dev_langiddesc(uint16_t *length);
uint8_t *usb_dev_manufacturerstr(uint16_t *length);
uint8_t *usb_dev_productdesc(uint16_t *length);
uint8_t *usb_dev_serialstr(uint16_t *length);
uint8_t *usb_dev_configstrdesc(uint16_t *length);
uint8_t *usb_dev_intfstrdesc(uint16_t *length);
uint8_t *usb_dev_winusbosstr(uint16_t *length);   // 新增：MS OS 字符串

// =============== 描述符回调表 ===============
usb_dev_desc_func user_desc = {
    &usb_dev_desc,
    &usb_dev_langiddesc,
    &usb_dev_manufacturerstr,
    &usb_dev_productdesc,
    &usb_dev_serialstr,
    &usb_dev_configstrdesc,
    &usb_dev_intfstrdesc,
    &usb_dev_winusbosstr,     // 第 8 项：启用 MS OS 1.0（0xEE），与 2.0(BOS) 同时可用
};

// =============== 设备描述符 ===============
__USB_ALIGN_BEGIN static uint8_t usb_dev_devicedesc[USB_SIZ_DEVICE_DESC] = {
    0x12,                       // bLength
    USB_DEVICE_DESCRIPTOR_TYPE, // bDescriptorType
    0x10,                       // bcdUSB 低字节（USB 2.10，支持 BOS）
    0x02,                       // bcdUSB 高字节
    0x00,                       // bDeviceClass（由接口描述符声明 0xFF 即可）
    0x00,                       // bDeviceSubClass
    0x00,                       // bDeviceProtocol
    USB_MAX_EP0_SIZE,           // bMaxPacketSize
    LOBYTE(DEV_VID),            // idVendor 低字节
    HIBYTE(DEV_VID),            // idVendor 高字节
    LOBYTE(DEV_PID),            // idProduct 低字节
    HIBYTE(DEV_PID),            // idProduct 高字节
    0x00,                       // bcdDevice 低字节（版本 2.00）
    0x02,
    MFC_STR_IDX,                // 制造商字符串索引
    PRODUCT_STR_IDX,            // 产品字符串索引
    SERIAL_STR_IDX,             // 序列号字符串索引
    DEV_MAX_CFG_NUM             // 配置数量
};

// =============== 语言 ID 描述符 ===============
__USB_ALIGN_BEGIN static uint8_t USB_DEV_LangIDDesc[USB_SIZ_STRING_LANGID] = {
    USB_SIZ_STRING_LANGID,
    USB_DESC_TYPE_STRING,
    LOBYTE(DEV_LANGID_STRING),
    HIBYTE(DEV_LANGID_STRING),
};

// =============== 字符串描述符临时缓冲 ===============
__USB_ALIGN_BEGIN static uint8_t usb_dev_strdesc[USB_MAX_STR_DESC_SIZ];

// =============== MS OS 字符串描述符（WCID 关键） ===============
// 作用：Windows 枚举时读 0xEE 字符串，得知"用 vendor 请求 0x01 继续查询"，
//       随后按请求返回 Compat ID（"WINUSB"），Windows 即自动绑定 WinUSB 驱动
__USB_ALIGN_BEGIN static uint8_t usb_dev_msosstr[] = {
    0x12, 0x03,                              // bLength=18, bDescriptorType=STRING
    'M', 0, 'S', 0, 'F', 0, 'T', 0,          // "MSFT100"（UTF-16LE）
    '1', 0, '0', 0, '0', 0,
    0x01,                                    // bMS_VendorCode（后续 vendor 请求码）
    0x00                                     // bPad
};

// =============== BOS 描述符（MS OS 2.0，真正的免驱） ===============
// 结构：BOS 头(5) + Windows Platform Capability(28) = 33 字节
__USB_ALIGN_BEGIN static uint8_t usb_dev_bosdesc[] = {
    0x05, 0x0F,             // bLength=5, bDescriptorType=BOS(0x0F)
    0x21, 0x00,             // wTotalLength = 33
    0x01,                   // bNumDeviceCaps = 1

    // ---- Windows Platform Capability ----
    0x1C,                   // bLength = 28
    0x10,                   // bDescriptorType = DEVICE_CAPABILITY(0x10)
    0x05,                   // bDevCapabilityType = PLATFORM(0x05)
    0x00,                   // bReserved
    // PlatformCapabilityUUID: {D8DD60DF-45D8-4289-95D4-F14F8E46E08A}
    0xDF, 0x60, 0xDD, 0xD8, 0xD8, 0x45, 0x89, 0x42,
    0x95, 0xD4, 0xF1, 0x4F, 0x8E, 0x46, 0xE0, 0x8A,
    0x00, 0x00, 0x03, 0x06, // dwWindowsVersion = 0x06030000（Win8.1，兼容性最好）
    0x2E, 0x00,             // wMSOSDescriptorSetTotalLength = 46
    0x01,                   // bMS_VendorCode = 0x01（用 vendor 请求 0x01 取描述符集）
    0x00                    // bAltEnumCode = 0
};

// =============== MS OS 2.0 描述符集（vendor 请求 0x01 / wIndex=0x07 返回） ===============
// 结构：Set Header(10) + Config Subset(8) + Function Subset(8) + Compatible ID(20) = 46 字节
__USB_ALIGN_BEGIN static uint8_t usb_dev_msos20[] = {
    // ---- Set Header ----
    0x0A, 0x00,             // wLength = 10
    0x00, 0x00,             // wDescriptorType = 0x00 (SET_HEADER)
    0x00, 0x00, 0x03, 0x06, // dwWindowsVersion = 0x06030000
    0x2E, 0x00,             // wTotalLength = 46

    // ---- Configuration Subset Header ----
    0x08, 0x00,             // wLength = 8
    0x01, 0x00,             // wDescriptorType = 0x01 (SUBSET_HEADER_CONFIG)
    0x01,                   // bConfigurationValue = 1
    0x00,                   // bReserved
    0x24, 0x00,             // wTotalLength = 36

    // ---- Function Subset Header ----
    0x08, 0x00,             // wLength = 8
    0x02, 0x00,             // wDescriptorType = 0x02 (SUBSET_HEADER_FUNCTION)
    0x00,                   // bFirstInterface = 0
    0x00,                   // bReserved
    0x1C, 0x00,             // wTotalLength = 28

    // ---- Compatible ID Feature Descriptor ----
    0x14, 0x00,             // wLength = 20
    0x03, 0x00,             // wDescriptorType = 0x03 (FEATURE_COMPATIBLE_ID)
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,  // CompatibleID = "WINUSB"
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // SubCompatibleID
};

// =============== 函数实现 ===============

// 返回设备描述符
uint8_t *usb_dev_desc(uint16_t *length)
{
    *length = (uint16_t)sizeof(usb_dev_devicedesc);
    return usb_dev_devicedesc;
}

// 返回语言 ID 描述符
uint8_t *usb_dev_langiddesc(uint16_t *length)
{
    *length = (uint16_t)sizeof(USB_DEV_LangIDDesc);
    return USB_DEV_LangIDDesc;
}

// 返回产品字符串描述符
uint8_t *usb_dev_productdesc(uint16_t *length)
{
    usb_getstring((uint8_t *)DEV_PRODUCT_FS_STRING, usb_dev_strdesc, length);
    return usb_dev_strdesc;
}

// 返回制造商字符串描述符
uint8_t *usb_dev_manufacturerstr(uint16_t *length)
{
    usb_getstring((uint8_t *)DEV_MANUFACTURER_STRING, usb_dev_strdesc, length);
    return usb_dev_strdesc;
}

// 返回序列号字符串描述符
uint8_t *usb_dev_serialstr(uint16_t *length)
{
    usb_getstring((uint8_t *)DEV_SERIALNUMBER_FS_STRING, usb_dev_strdesc, length);
    return usb_dev_strdesc;
}

// 返回配置字符串描述符
uint8_t *usb_dev_configstrdesc(uint16_t *length)
{
    usb_getstring((uint8_t *)DEV_CONFIGURATION_FS_STRING, usb_dev_strdesc, length);
    return usb_dev_strdesc;
}

// 返回接口字符串描述符
uint8_t *usb_dev_intfstrdesc(uint16_t *length)
{
    usb_getstring((uint8_t *)DEV_INTERFACE_FS_STRING, usb_dev_strdesc, length);
    return usb_dev_strdesc;
}

// 返回 MS OS 字符串描述符（Windows 探测 WCID 1.0 用，保留兼容）
uint8_t *usb_dev_winusbosstr(uint16_t *length)
{
    *length = (uint16_t)sizeof(usb_dev_msosstr);
    return usb_dev_msosstr;
}

// 返回 BOS 描述符（GET_DESCRIPTOR BOS 请求）
uint8_t *usb_dev_getbos(uint16_t *length)
{
    *length = (uint16_t)sizeof(usb_dev_bosdesc);
    return usb_dev_bosdesc;
}

// 返回 Device Capability 描述符（BOS 头之后的部分，28 字节）
uint8_t *usb_dev_getdevcap(uint16_t *length)
{
    *length = (uint16_t)(sizeof(usb_dev_bosdesc) - 5U);
    return &usb_dev_bosdesc[5];
}

// 返回 MS OS 2.0 描述符集（vendor 请求 wIndex=0x07）
uint8_t *usb_dev_getmsos20(uint16_t *length)
{
    *length = (uint16_t)sizeof(usb_dev_msos20);
    return usb_dev_msos20;
}
