#ifndef RS485_H
#define RS485_H

#include "hc32_ll.h"
#include "rtthread.h"

// RS485 引脚和 USART 定义
#define RS485_UNIT              CM_USART4
#define RS485_FCG_ENABLE()      FCG_Fcg1PeriphClockCmd(FCG1_PERIPH_USART4, ENABLE)

#define RS485_RX_PORT           GPIO_PORT_B
#define RS485_RX_PIN            GPIO_PIN_09
#define RS485_RX_FUNC           GPIO_FUNC_37

#define RS485_TX_PORT           GPIO_PORT_B
#define RS485_TX_PIN            GPIO_PIN_07
#define RS485_TX_FUNC           GPIO_FUNC_36

// RS485 方向控制引脚
#define RS485_CS_PORT           GPIO_PORT_B
#define RS485_CS_PIN            GPIO_PIN_08
#define RS485_CS_OFF()          GPIO_SetPins(RS485_CS_PORT, RS485_CS_PIN)
#define RS485_CS_ON()           GPIO_ResetPins(RS485_CS_PORT, RS485_CS_PIN)

// USART 中断号和中断源
#define RS485_TX_CPLT_IRQn              (INT005_IRQn)
#define RS485_TX_CPLT_INT_SRC           (INT_SRC_USART4_TCI)

#define RS485_RX_ERR_IRQn               (INT006_IRQn)
#define RS485_RX_ERR_INT_SRC            (INT_SRC_USART4_EI)

#define RS485_RX_TIMEOUT_IRQn           (INT007_IRQn)
#define RS485_RX_TIMEOUT_INT_SRC        (INT_SRC_USART4_RTO)

// ================== RX DMA ==================
#define RS485_RX_DMA_UNIT               (CM_DMA1)
#define RS485_RX_DMA_CH                 (DMA_CH0)
#define RS485_RX_DMA_FCG_ENABLE()       (FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_DMA1, ENABLE))
// AOS 路由: USART RX 事件 → DMA 触发请求 (HC32 DMA 必须通过 AOS 配触发源)
#define RS485_RX_DMA_TRIG_SEL           (AOS_DMA1_0)
#define RS485_RX_DMA_TRIG_EVT_SRC       (EVT_SRC_USART4_RI)

// ================== TX DMA (与 RX 共用 DMA1, 通道1) ==================
#define RS485_TX_DMA_UNIT               (CM_DMA1)
#define RS485_TX_DMA_CH                 (DMA_CH1)
// AOS 路由: USART TX 事件 → DMA 触发请求
#define RS485_TX_DMA_TRIG_SEL           (AOS_DMA1_1)
#define RS485_TX_DMA_TRIG_EVT_SRC       (EVT_SRC_USART4_TI)
// TX DMA 传输完成中断
#define RS485_TX_DMA_TC_INT             (DMA_INT_TC_CH1)
#define RS485_TX_DMA_TC_FLAG            (DMA_FLAG_TC_CH1)
#define RS485_TX_DMA_TC_IRQn            (INT001_IRQn)
#define RS485_TX_DMA_TC_INT_SRC         (INT_SRC_DMA1_TC1)

// RX 缓冲区长度
#define RS485_RX_BUF_LEN        4096U

// TMR0 单元2通道B: 用作 USART RX 超时定时器, 判定不定长帧结束
#define TMR0_UNIT               CM_TMR0_2
#define TMR0_CH                 TMR0_CH_B
#define TMR0_FCG_ENABLE()       (FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR0_2, ENABLE))

// RS485 配置结构体
typedef struct {
    uint32_t rs485_baudrate;     // 波特率
    uint8_t  data_width;         // 数据位宽度: 1=9位, 其它=8位
    uint8_t  stop_bits;          // 停止位数: 1=2位, 其它=1位
    uint8_t  parity;             // 校验位: 1=奇校验, 2=偶校验, 其它=无校验
    uint8_t  over_sample_bit;    // 超采样位数: 1=16倍, 其它=8倍
    uint8_t  first_bit;          // 起始位: 1=高位在前, 其它=低位在前
    uint8_t  start_bit_polarity; // 起始位检测: 1=下降沿, 其它=低电平
    uint8_t  hw_flow_control;    // 硬件流控: 1=CTS, 其它=RTS(HC32上等价于关闭)
    uint16_t rs485_timeout_bits; // 超时时间, 单位: 位
} stc_rs485_config_t;

extern uint8_t rs485_rx_buf[];
extern stc_rs485_config_t rs485_config;

// 帧长 (超时中断中赋值, 上层读取)
extern volatile uint16_t rs485_rx_frame_len;

extern rt_sem_t rx_frame_done_sem;

/**
 * @brief 初始化 RS485 (GPIO + DMA + USART + 中断 + 信号量)
 */
void rs485_init(void);

/**
 * @brief 重装 RX DMA, 重新开始接收 (应用层处理完一帧后调用)
 */
void rs485_rx_reload(void);

/**
 * @brief DMA 方式发送数据
 * @param pData  待发送数据地址
 * @param u16Len 待发送数据长度 (字节)
 */
void rs485_tx_dma(uint8_t *pData, uint16_t u16Len);


#endif
