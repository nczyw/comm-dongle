// USB 设备描述符 + 字符串描述符（WinUSB 自定义类）
// 说明：本文件额外提供 MS OS(0xEE) 字符串描述符，配合 usb_dev_winusb_class，
//       让 Windows 枚举时自动绑定 WinUSB 驱动（免驱）

#include "usb_dev_desc.h"
#include "usb_dev_stdreq.h"
#include "at24c512.h"
#include "hc32_ll_efm.h"
#include <string.h>

// =============== 设备 VID / PID ===============
// 注意：不要和已有设备冲突
#define DEV_VID                        (0x2E88)
#define DEV_PID                        (0x4603)

// 语言 ID（字符串描述符用）
#define DEV_LANGID_STRING              (0x409)              // 英语（美国）

// =============== 函数前置声明 ===============
uint8_t *usb_dev_desc(uint16_t *length);
uint8_t *usb_dev_langiddesc(uint16_t *length);
uint8_t *usb_dev_manufacturerstr(uint16_t *length);
uint8_t *usb_dev_productdesc(uint16_t *length);
uint8_t *usb_dev_serialstr(uint16_t *length);
uint8_t *usb_dev_configstrdesc(uint16_t *length);
uint8_t *usb_dev_intfstrdesc(uint16_t *length);
uint8_t *usb_dev_winusbosstr(uint16_t *length);   // MS OS 1.0（0xEE）字符串

// =============== 描述符回调表 ===============
usb_dev_desc_func user_desc = {
    &usb_dev_desc,
    &usb_dev_langiddesc,
    &usb_dev_manufacturerstr,
    &usb_dev_productdesc,
    &usb_dev_serialstr,
    &usb_dev_configstrdesc,
    &usb_dev_intfstrdesc,
    &usb_dev_winusbosstr,     // 第 8 项：启用 MS OS 1.0（0xEE），实测最能被 Windows 采纳
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

// =============== 从 EEPROM 加载的描述符字符串（RAM 缓存） ===============
// getter 只读这里，绝不在此处做 I2C（getter 运行在 USB 中断上下文）
static usb_desc_cfg_t g_desc_cfg;

// =============== MS OS 字符串描述符（WCID 1.0，免驱主力） ===============
// Windows 枚举时读 0xEE 字符串，得知"用 vendor 请求 0x01 继续查询"，
// 随后按请求返回 Compat ID（"WINUSB"），Windows 即自动绑定 WinUSB 驱动
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
    usb_getstring((uint8_t *)g_desc_cfg.prod, usb_dev_strdesc, length);
    return usb_dev_strdesc;
}

// 返回制造商字符串描述符
uint8_t *usb_dev_manufacturerstr(uint16_t *length)
{
    usb_getstring((uint8_t *)g_desc_cfg.man, usb_dev_strdesc, length);
    return usb_dev_strdesc;
}

// 返回序列号字符串描述符
uint8_t *usb_dev_serialstr(uint16_t *length)
{
    usb_getstring((uint8_t *)g_desc_cfg.sn, usb_dev_strdesc, length);
    return usb_dev_strdesc;
}

// 返回配置字符串描述符
uint8_t *usb_dev_configstrdesc(uint16_t *length)
{
    usb_getstring((uint8_t *)g_desc_cfg.cfg, usb_dev_strdesc, length);
    return usb_dev_strdesc;
}

// 返回接口字符串描述符
uint8_t *usb_dev_intfstrdesc(uint16_t *length)
{
    usb_getstring((uint8_t *)g_desc_cfg.intf, usb_dev_strdesc, length);
    return usb_dev_strdesc;
}

// 返回 MS OS 字符串描述符（Windows 探测 WCID 1.0 用）
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

// =============== EEPROM 描述符字符串的读写 ===============

// 拷贝字符串到定长字段，写入长度并补 '\0'
static void desc_copy(uint8_t *len, char *dst, uint8_t max, const char *src)
{
    uint8_t n = 0U;

    while ((src[n] != '\0') && (n < max)) {
        n++;
    }
    (void)memcpy(dst, src, n);
    dst[n] = '\0';
    *len = n;
}

// CRC16-CCITT（poly 0x1021, init 0xFFFF）
static uint16_t desc_crc16(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFFU;
    uint32_t i;
    uint8_t  b;

    for (i = 0U; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8U);
        for (b = 0U; b < 8U; b++) {
            if ((crc & 0x8000U) != 0U) {
                crc = (uint16_t)((crc << 1U) ^ 0x1021U);
            } else {
                crc = (uint16_t)(crc << 1U);
            }
        }
    }
    return crc;
}

// 用芯片唯一 ID（96bit）生成 24 位十六进制默认序列号
// 输出顺序：u32UniqueID2 -> u32UniqueID1 -> u32UniqueID0（每个字内部小端，低字节在前）
// 与 UID 严格一一映射，不做压缩，确保每片唯一、同片稳定
static void uid_to_serial(char *out)
{
    static const char hex[] = "0123456789ABCDEF";
    stc_efm_unique_id_t uid;
    uint32_t words[3];
    uint8_t  i;
    uint8_t  j;
    uint8_t  k = 0U;

    EFM_GetUID(&uid);
    words[0] = uid.u32UniqueID2;
    words[1] = uid.u32UniqueID1;
    words[2] = uid.u32UniqueID0;

    for (i = 0U; i < 3U; i++) {
        for (j = 0U; j < 4U; j++) {
            uint8_t byte = (uint8_t)(words[i] >> ((3U - j) * 8U));   // 大端：高字节先
            out[k++] = hex[(byte >> 4) & 0x0FU];
            out[k++] = hex[byte & 0x0FU];
        }
    }
    out[24U] = '\0';
}

// 载入默认值（EEPROM 为空或数据无效时）
static void desc_apply_default(void)
{
    (void)memset(&g_desc_cfg, 0, sizeof(g_desc_cfg));
    g_desc_cfg.magic   = USB_DESC_MAGIC;
    g_desc_cfg.version = USB_DESC_VERSION;

    desc_copy(&g_desc_cfg.len_man,  g_desc_cfg.man,  USB_DESC_LEN_MAN,  USB_DESC_DEF_MAN);
    desc_copy(&g_desc_cfg.len_prod, g_desc_cfg.prod, USB_DESC_LEN_PROD, USB_DESC_DEF_PROD);

    uid_to_serial(g_desc_cfg.sn);               // 默认序列号：芯片 UID 派生
    g_desc_cfg.len_sn = (uint8_t)strlen(g_desc_cfg.sn);

    desc_copy(&g_desc_cfg.len_cfg,  g_desc_cfg.cfg,  USB_DESC_LEN_CFG,  USB_DESC_DEF_CFG);
    desc_copy(&g_desc_cfg.len_intf, g_desc_cfg.intf, USB_DESC_LEN_INTF, USB_DESC_DEF_INTF);
}

void usb_desc_cfg_load(void)
{
    usb_desc_cfg_t tmp;
    uint16_t crc;
    uint8_t  ok = 0U;

    if (at24c512_read(USB_DESC_EEPROM_ADDR, (uint8_t *)&tmp, sizeof(tmp)) == LL_OK) {
        if ((tmp.magic == USB_DESC_MAGIC) && (tmp.version == USB_DESC_VERSION)) {
            crc = desc_crc16((const uint8_t *)&tmp,
                             (uint32_t)(sizeof(tmp) - sizeof(uint16_t)));
            if (crc == tmp.crc) {
                // 长度必须合法
                if ((tmp.len_man  <= USB_DESC_LEN_MAN)  &&
                    (tmp.len_prod <= USB_DESC_LEN_PROD) &&
                    (tmp.len_sn   <= USB_DESC_LEN_SN)   &&
                    (tmp.len_cfg  <= USB_DESC_LEN_CFG)  &&
                    (tmp.len_intf <= USB_DESC_LEN_INTF)) {
                    // 强制 '\0' 结尾，供 usb_getstring 使用
                    tmp.man[tmp.len_man]   = '\0';
                    tmp.prod[tmp.len_prod] = '\0';
                    tmp.sn[tmp.len_sn]     = '\0';
                    tmp.cfg[tmp.len_cfg]   = '\0';
                    tmp.intf[tmp.len_intf] = '\0';
                    g_desc_cfg = tmp;
                    ok = 1U;
                }
            }
        }
    }

    if (ok == 0U) {
        desc_apply_default();                   // 新 EEPROM(全 0xFF)/损坏 -> 默认值
    }
}

int32_t usb_desc_cfg_save(void)
{
    uint16_t crc;

    g_desc_cfg.magic    = USB_DESC_MAGIC;
    g_desc_cfg.version  = USB_DESC_VERSION;
    g_desc_cfg.reserved = 0U;
    crc = desc_crc16((const uint8_t *)&g_desc_cfg,
                     (uint32_t)(sizeof(g_desc_cfg) - sizeof(uint16_t)));
    g_desc_cfg.crc = crc;

    return at24c512_wirte(USB_DESC_EEPROM_ADDR,
                          (const uint8_t *)&g_desc_cfg, sizeof(g_desc_cfg));
}
