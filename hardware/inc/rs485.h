#ifndef RS485_H
#define RS485_H

#include "hc32_ll.h"


// RS485 配置参数
#define RS485_UNIT              CM_USART4
#define RS485_FCG_ENABLE()      FCG_Fcg1PeriphClockCmd(FCG1_PERIPH_USART4, ENABLE)

#define RS485_RX_PORT           GPIO_PORT_B
#define RS485_RX_PIN            GPIO_PIN_09
#define RS485_RX_FUNC           GPIO_FUNC_37

#define RS485_TX_PORT           GPIO_PORT_B
#define RS485_TX_PIN            GPIO_PIN_07
#define RS485_TX_FUNC           GPIO_FUNC_36


#define RS485_CS_PORT           GPIO_PORT_B
#define RS485_CS_PIN            GPIO_PIN_08
#define RS485_CS_OFF()          GPIO_SetPins(RS485_CS_PORT, CS_PIN)
#define RS485_CS_ON()           GPIO_ResetPins(RS485_CS_PORT, CS_PIN)

#define RS485_TX_CPLT_IRQn              (INT005_IRQn)
#define RS485_TX_CPLT_INT_SRC           (INT_SRC_USART4_TCI)

#define RS485_RX_ERR_IRQn               (INT006_IRQn)
#define RS485_RX_ERR_INT_SRC            (INT_SRC_USART4_EI)

#define RS485_RX_TIMEOUT_IRQn           (INT007_IRQn)
#define RS485_RX_TIMEOUT_INT_SRC        (INT_SRC_USART4_RTO)

// RS485 接收DMA配置
#define RS485_RX_DMA_UNIT               (CM_DMA1)
#define RS485_RX_DMA_CH                 (DMA_CH0)
#define RS485_RX_DMA_FCG_ENABLE()       (FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_DMA1, ENABLE))
#define RS485_RX_DMA_TC_INT_SRC         (INT_SRC_DMA1_TC0)
#define RS485_RX_DMA_TC_IRQn            (INT008_IRQn)
#define RS485_RX_DMA_TC_FLAG            (DMA_FLAG_TC_CH0)
#define RS485_RX_DMA_TC_INT             (DMA_INT_TC_CH0)

#define RS485_RX_BUF_LEN        4096U

// 超时中断配置, 用于不定长接收, 但是缓存空间,只有4096字节
// 定时器0单元2通道B
#define TMR0_UNIT               CM_TMR0_2
#define TMR0_CH                 TMR0_CH_B
#define TMR0_FCG_ENABLE()       (FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMR0_2, ENABLE))

typedef struct {
    uint32_t rs485_baudrate;     // 波特率
    uint8_t  data_width;         // 数据位宽度, 1: 9位, 其它值: 8位
    uint8_t  stop_bits;          // 停止位数, 1: 2位, 其它值: 1位
    uint8_t  parity;             // 校验位, 1: 奇校验(even), 2: 偶校验(odd), 其它值: 无校验(none)
    uint8_t  over_sample_bit;    // 超采样位数, 1: 16位, 其它值: 8位
    uint8_t  first_bit;          // 开始位, 1: 高位在前, 其它值: 低位在前
    uint8_t  start_bit_polarity; // 边沿启始方式, 1: 下降沿检测, 其它值: 低就电平检测
    uint8_t  hw_flow_control;    // 硬件控制方式: 1: RTS, 其它值: CTS
    uint16_t rs485_timeout_bits; // 超时时间, 单位: 位
} stc_rs485_config_t;


extern uint8_t rs485_rx_buf[];
extern stc_rs485_config_t rs485_config;


void rs485_init(void);


#endif
