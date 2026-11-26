#ifndef CAN_H
#define CAN_H

#include "hc32_ll.h"

#define CAN_UNIT                (CM_CAN2)
#define CAN_PERIPH_CLK          (FCG1_PERIPH_CAN2)

// CAN 发送引脚定义
#define CAN_TX_PORT             (GPIO_PORT_B)
#define CAN_TX_PIN              (GPIO_PIN_03)
#define CAN_TX_PIN_FUNC         (GPIO_FUNC_60)

// CAN 接收引脚定义
#define CAN_RX_PORT             (GPIO_PORT_B)
#define CAN_RX_PIN              (GPIO_PIN_04)
#define CAN_RX_PIN_FUNC         (GPIO_FUNC_61)

// CAN 时钟引脚定义
#define CAN_CLK_UNIT            (CLK_CAN2)
//#define CAN_CLK_SRC             (CLK_CANCLK_SYSCLK_DIV2)

// CAN 中断定义
#define CAN_INT_PRIO            (DDL_IRQ_PRIO_00)
#define CAN_INT_IRQn            (CAN2_INT_IRQn)
#define CAN_IRQ_HANDLE          CAN2_Handler

// CAN 中断类型选择
#define CAN_INT_SEL             (CAN_INT_PTB_TX | CAN_INT_RX | CAN_INT_ERR_INT)

// 过滤器最大上限
#define CAN_FILTER_MAX          (16)

// CAN 配置在 EEPROM 中的起始地址（按实际分区规划调整）
#define CAN_EEPROM_BASE_ADDR    (0x0000U)

// 经典 CAN / CAN-FD 仲裁段 通用波特率配置
typedef struct __attribute__((packed)) {
    uint32_t prescaler;
    uint32_t time_seg1;
    uint32_t time_seg2;
    uint32_t sjw;
} can_baud_cfg_t;

// CAN-FD 专用配置（可选：.fd_enable==0 时整个块忽略）
typedef struct __attribute__((packed)) {
    can_baud_cfg_t  fast;           // 数据段快速波特率（用 FBT 规则）

    uint8_t         fd_enable;      // 0=关闭 CAN-FD   1=启用
    uint8_t         fd_mode;        // 0=博世   1=ISO   (禁止存驱动枚举)
    uint8_t         tdc_enable;     // 发射延迟补偿   0/1
    uint8_t         rfu[1];         // 保留字段（SSP offset 直接取 fast.time_seg1）
} can_fd_cfg_t;

// 单条过滤器条目
typedef struct __attribute__((packed)) {
    uint32_t    id;         // 原始数值，标准帧 11bit 左对齐 / 扩展帧 29bit 左对齐
    uint32_t    mask;       // bit=1 该位忽略（不比较），bit=0 必须与 id 匹配
    uint8_t     id_type;    // 0=标准帧  1=扩展帧  2=双匹配（禁止存驱动枚举）
    uint8_t     rfu[3];     // 保留字段
} can_filter_entry_t;


// eeprom 配置结构体
typedef struct __attribute__((packed)) {
    can_filter_entry_t  filter[CAN_FILTER_MAX]; // 过滤器条目
    can_baud_cfg_t      classic;                // 经典 CAN 波特率
    can_fd_cfg_t        fd;                     // CAN-FD 波特率

    uint8_t             can_clk_src;            // CAN 时钟源
    uint8_t             filter_count;           // 实际启用条目数（<=CAN_FILTER_MAX） 
    uint8_t             work_mode;              // 0=Normal 1=Silent 2=ILB 3=ELB
    uint8_t             rfu[1];                  // 保留字段
} can_eeprom_cfg_t;

int32_t can_cfg_load(can_eeprom_cfg_t *cfg);            // EEPROM -> 内存，并校验
int32_t can_cfg_save(const can_eeprom_cfg_t *cfg);      // 校验后 内存 -> EEPROM

void can_init_config(const can_eeprom_cfg_t *pstcCfg);         // 初始化 CAN 配置

void can_init(void);

#endif