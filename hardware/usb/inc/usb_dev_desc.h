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

// =============== 描述符字符串的 EEPROM 存储 ===============
// 位置：AT24C512 最后两页（0xFF00 ~ 0xFFFF，共 256 字节）
// CAN 配置在 0x0000，二者不重叠
#define USB_DESC_EEPROM_ADDR        (0xFF00U)
#define USB_DESC_MAGIC              (0x574AU)   // 魔数（'W''J'），用于识别有效数据
#define USB_DESC_VERSION            (0x01U)     // 存储格式版本

// 各字符串最大长度（不含结尾 '\0'）
#define USB_DESC_LEN_MAN            (31U)
#define USB_DESC_LEN_PROD           (63U)
#define USB_DESC_LEN_SN             (31U)
#define USB_DESC_LEN_CFG            (31U)
#define USB_DESC_LEN_INTF           (47U)

// 默认值（EEPROM 为空/损坏时使用）
#define USB_DESC_DEF_MAN            "XHSC"
#define USB_DESC_DEF_PROD           "USB-Box"
#define USB_DESC_DEF_CFG            "USB-Box Config"
#define USB_DESC_DEF_INTF           "USB-Box Interface"

// EEPROM 中的存储结构（packed，固定布局，共 219 字节）
typedef struct __attribute__((packed)) {
    uint16_t magic;                                 // 魔数
    uint8_t  version;                               // 版本
    uint8_t  reserved;                              // 保留
    uint8_t  len_man;   char man[USB_DESC_LEN_MAN  + 1U];   // 制造商
    uint8_t  len_prod;  char prod[USB_DESC_LEN_PROD + 1U];  // 产品名
    uint8_t  len_sn;    char sn[USB_DESC_LEN_SN    + 1U];   // 序列号
    uint8_t  len_cfg;   char cfg[USB_DESC_LEN_CFG  + 1U];   // 配置名
    uint8_t  len_intf;  char intf[USB_DESC_LEN_INTF + 1U];  // 接口名
    uint16_t crc;                                   // 前面所有字节的 CRC16
} usb_desc_cfg_t;

// 描述符回调表（定义在 usb_dev_desc.c）
extern usb_dev_desc_func user_desc;

// 从 EEPROM 加载描述符字符串到 RAM 缓存
// 必须在 USB 初始化前、任务上下文调用；数据无效时自动使用默认值
void usb_desc_cfg_load(void);

// 把当前 RAM 缓存的描述符字符串写入 EEPROM（任务上下文调用）
int32_t usb_desc_cfg_save(void);

#ifdef __cplusplus
}
#endif

#endif // __USB_DEV_DESC_H__
