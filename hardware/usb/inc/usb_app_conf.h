#ifndef USB_APP_CONF_H
#define USB_APP_CONF_H

// C++ 编译时按 C 方式链接
#ifdef __cplusplus
extern "C"
{
#endif

// =============== USB 模式配置 =============== 

// 全速模式（12Mbps）
#define USB_FS_MODE
// 设备模式
#define USE_DEVICE_MODE

#ifndef USB_FS_MODE
#define USB_FS_MODE
#endif

#ifndef USE_DEVICE_MODE
#ifndef USE_HOST_MODE
#error  "USE_DEVICE_MODE or USE_HOST_MODE should be defined"
#endif
#endif

// =============== USB 端点配置（WinUSB 自定义类） =============== 

// EP1 IN（Bulk，设备 -> 主机）
#define WINUSB_IN_EP            (0x81U)
// EP1 OUT（Bulk，主机 -> 设备）
#define WINUSB_OUT_EP           (0x01U)
// IN/OUT 端点单包最大字节数（全速 Bulk 最大 64）
#define MAX_WINUSB_PACKET_SIZE  (64U)

// =============== USB FIFO 分配 =============== 
// 说明：EP0-IN 用 TX0，EP1-IN(0x81) 用 TX1，所有 OUT 端点共用 RX
// 单位是 4 字节，总和不能超过 320

// 接收 FIFO（所有 OUT 端点共用）
#define RX_FIFO_FS_SIZE         (128U)
// EP0 IN 的 FIFO
#define TX0_FIFO_FS_SIZE        (64U)
// EP1 IN 的 FIFO（WinUSB 数据发往主机）
#define TX1_FIFO_FS_SIZE        (64U)
#define TX2_FIFO_FS_SIZE        (0U)
#define TX3_FIFO_FS_SIZE        (0U)
#define TX4_FIFO_FS_SIZE        (0U)
#define TX5_FIFO_FS_SIZE        (0U)

#if ((RX_FIFO_FS_SIZE + TX0_FIFO_FS_SIZE + TX1_FIFO_FS_SIZE + TX2_FIFO_FS_SIZE + TX3_FIFO_FS_SIZE + TX4_FIFO_FS_SIZE + \
      TX5_FIFO_FS_SIZE) > 320U)
#error  "The USB max FIFO size is 320 x 4 Bytes!"
#endif

// =============== 功能配置 ===============

// 配置描述符数量
#define DEV_MAX_CFG_NUM         (1U)
// 接口数量（WinUSB 只有 1 个接口）
#define USBD_ITF_MAX_NUM        (1U)
// 字符串描述符最大长度
#define USB_MAX_STR_DESC_SIZ    (128U)

// =============== VBUS 检测 ===============
// 本板未接 PA09（USB 只用 PA11/PA12），关闭 VBUS 检测
//#define VBUS_SENSING_ENABLED

// 自供电（需要时再打开）
//#define SELF_POWER

#ifdef __cplusplus
}
#endif

#endif // USB_APP_CONF_H
