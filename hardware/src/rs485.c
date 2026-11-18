#include "rs485.h"

/*******************************************************************************
 * 全局变量
 ******************************************************************************/
uint8_t rs485_rx_buf[RS485_RX_BUF_LEN];
stc_rs485_config_t rs485_config;

// 帧长 (超时中断赋值, 上层读取)
volatile uint16_t rs485_rx_frame_len;

// 接收完成信号量 (超时中断释放, 上层线程 rt_sem_take 等待)
rt_sem_t rx_frame_done_sem = RT_NULL;

/*******************************************************************************
 * 本地函数声明
 ******************************************************************************/
static int32_t dma_init(void);
static void tmr0_config(uint16_t timeout_bits);
static stc_usart_uart_init_t rs485_data_init(void);
static void rs485_tx_complete_irq_callback(void);
static void rs485_rx_error_irq_callback(void);
static void rs485_rx_timeout_irq_callback(void);
static void rs485_tx_dma_tc_irq_callback(void);

/*******************************************************************************
 * 函数实现
 ******************************************************************************/

/**
 * @brief  TX DMA 传输完成中断回调
 */
static void rs485_tx_dma_tc_irq_callback(void)
{
    // 开 USART 发送完成中断, 等 shift register 空了再切回 RS485 接收
    USART_FuncCmd(RS485_UNIT, USART_INT_TX_CPLT, ENABLE);

    DMA_ClearTransCompleteStatus(RS485_TX_DMA_UNIT, RS485_TX_DMA_TC_FLAG);
}

/**
 * @brief  DMA 初始化 (RX + TX)
 * @retval int32_t: LL_OK 或错误码
 */
static int32_t dma_init(void)
{
    int32_t i32Ret;
    stc_dma_init_t stcDmaInit;
    stc_irq_signin_config_t stcIrqSignConfig;

    // 开 DMA1 和 AOS 时钟 (RX CH0 + TX CH1 共用 DMA1)
    RS485_RX_DMA_FCG_ENABLE();
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

    // ================== USART RX DMA ==================
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn       = DMA_INT_DISABLE;
    stcDmaInit.u32BlockSize   = 1UL;
    stcDmaInit.u32TransCount  = ARRAY_SZ(rs485_rx_buf);
    stcDmaInit.u32DataWidth   = DMA_DATAWIDTH_8BIT;
    stcDmaInit.u32DestAddr    = (uint32_t)rs485_rx_buf;
    stcDmaInit.u32SrcAddr     = (uint32_t)(&RS485_UNIT->RDR);
    stcDmaInit.u32SrcAddrInc  = DMA_SRC_ADDR_FIX;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_INC;
    i32Ret = DMA_Init(RS485_RX_DMA_UNIT, RS485_RX_DMA_CH, &stcDmaInit);
    if (LL_OK == i32Ret) {
        // 通过 AOS 路由 USART RX 事件到 DMA 触发源
        AOS_SetTriggerEventSrc(RS485_RX_DMA_TRIG_SEL, RS485_RX_DMA_TRIG_EVT_SRC);

        DMA_Cmd(RS485_RX_DMA_UNIT, ENABLE);
        (void)DMA_ChCmd(RS485_RX_DMA_UNIT, RS485_RX_DMA_CH, ENABLE);
    }

    // ================== USART TX DMA ==================
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn       = DMA_INT_ENABLE;
    stcDmaInit.u32BlockSize   = 1UL;
    stcDmaInit.u32TransCount  = ARRAY_SZ(rs485_rx_buf);
    stcDmaInit.u32DataWidth   = DMA_DATAWIDTH_8BIT;
    stcDmaInit.u32DestAddr    = (uint32_t)(&RS485_UNIT->TDR);
    stcDmaInit.u32SrcAddr     = (uint32_t)rs485_rx_buf;
    stcDmaInit.u32SrcAddrInc  = DMA_SRC_ADDR_INC;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_FIX;
    i32Ret = DMA_Init(RS485_TX_DMA_UNIT, RS485_TX_DMA_CH, &stcDmaInit);
    if (LL_OK == i32Ret) {
        // 注册 TX DMA 传输完成中断
        stcIrqSignConfig.enIntSrc   = RS485_TX_DMA_TC_INT_SRC;
        stcIrqSignConfig.enIRQn     = RS485_TX_DMA_TC_IRQn;
        stcIrqSignConfig.pfnCallback = &rs485_tx_dma_tc_irq_callback;
        (void)INTC_IrqSignIn(&stcIrqSignConfig);
        NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
        NVIC_SetPriority(stcIrqSignConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
        NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);

        // 通过 AOS 路由 USART TX 事件到 DMA 触发源
        AOS_SetTriggerEventSrc(RS485_TX_DMA_TRIG_SEL, RS485_TX_DMA_TRIG_EVT_SRC);

        DMA_Cmd(RS485_TX_DMA_UNIT, ENABLE);
        DMA_TransCompleteIntCmd(RS485_TX_DMA_UNIT, RS485_TX_DMA_TC_INT, ENABLE);
    }

    return i32Ret;
}

/**
 * @brief 配置 TMR0 作为 USART RX 超时定时器
 */
static void tmr0_config(uint16_t timeout_bits)
{
    uint16_t u16Div;
    uint16_t u16Delay;
    uint16_t u16CompareValue;
    stc_tmr0_init_t stcTmr0Init;

    TMR0_FCG_ENABLE();

    stcTmr0Init.u32ClockSrc = TMR0_CLK_SRC_XTAL32;
    stcTmr0Init.u32ClockDiv = TMR0_CLK_DIV8;
    stcTmr0Init.u32Func     = TMR0_FUNC_CMP;
    if (TMR0_CLK_DIV1 == stcTmr0Init.u32ClockDiv) {
        u16Delay = 7U;
    } else if (TMR0_CLK_DIV2 == stcTmr0Init.u32ClockDiv) {
        u16Delay = 5U;
    } else if ((TMR0_CLK_DIV4 == stcTmr0Init.u32ClockDiv) || \
               (TMR0_CLK_DIV8 == stcTmr0Init.u32ClockDiv) || \
               (TMR0_CLK_DIV16 == stcTmr0Init.u32ClockDiv)) {
        u16Delay = 3U;
    } else {
        u16Delay = 2U;
    }

    u16Div = (uint16_t)1U << (stcTmr0Init.u32ClockDiv >> TMR0_BCONR_CKDIVA_POS);
    u16CompareValue = ((timeout_bits + u16Div - 1U) / u16Div) - u16Delay;
    stcTmr0Init.u16CompareValue = u16CompareValue;
    (void)TMR0_Init(TMR0_UNIT, TMR0_CH, &stcTmr0Init);

    // 硬件启动/清零条件: USART 触发
    TMR0_HWStartCondCmd(TMR0_UNIT, TMR0_CH, ENABLE);
    TMR0_HWClearCondCmd(TMR0_UNIT, TMR0_CH, ENABLE);
}

/**
 * @brief 重装 RX DMA, 重新开始接收 (应用层处理完一帧后调用)
 */
void rs485_rx_reload(void)
{
    // 重装 DMA 参数
    DMA_SetDestAddr(RS485_RX_DMA_UNIT, RS485_RX_DMA_CH, (uint32_t)rs485_rx_buf);
    DMA_SetTransCount(RS485_RX_DMA_UNIT, RS485_RX_DMA_CH, ARRAY_SZ(rs485_rx_buf));

    // 开 DMA 通道 + 开 USART RX, 恢复接收
    (void)DMA_ChCmd(RS485_RX_DMA_UNIT, RS485_RX_DMA_CH, ENABLE);
    USART_FuncCmd(RS485_UNIT, USART_RX, ENABLE);

    // 清帧长
    rs485_rx_frame_len = 0U;
}

/**
 * @brief DMA 方式发送数据
 */
void rs485_tx_dma(uint8_t *pData, uint16_t u16Len)
{
    DMA_SetSrcAddr(RS485_TX_DMA_UNIT, RS485_TX_DMA_CH, (uint32_t)pData);
    DMA_SetTransCount(RS485_TX_DMA_UNIT, RS485_TX_DMA_CH, u16Len);

    (void)DMA_ChCmd(RS485_TX_DMA_UNIT, RS485_TX_DMA_CH, ENABLE);

    RS485_CS_ON();   // RS485 方向切为发送
    USART_FuncCmd(RS485_UNIT, USART_TX, ENABLE);
}

/**
 * @brief 将 rs485_config 解析为 USART 初始化结构体
 */
static stc_usart_uart_init_t rs485_data_init(void)
{
    stc_usart_uart_init_t data;
    data.u32Baudrate = rs485_config.rs485_baudrate;

    if (rs485_config.data_width == 1) {
        data.u32DataWidth = USART_DATA_WIDTH_9BIT;
    } else {
        data.u32DataWidth = USART_DATA_WIDTH_8BIT;
    }

    if (rs485_config.stop_bits == 1) {
        data.u32StopBit = USART_STOPBIT_2BIT;
    } else {
        data.u32StopBit = USART_STOPBIT_1BIT;
    }

    if (rs485_config.parity == 1) {
        data.u32Parity = USART_PARITY_EVEN;
    } else if (rs485_config.parity == 2) {
        data.u32Parity = USART_PARITY_ODD;
    } else {
        data.u32Parity = USART_PARITY_NONE;
    }

    if (rs485_config.over_sample_bit == 1) {
        data.u32OverSampleBit = USART_OVER_SAMPLE_16BIT;
    } else {
        data.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    }

    if (rs485_config.first_bit == 1) {
        data.u32FirstBit = USART_FIRST_BIT_MSB;
    } else {
        data.u32FirstBit = USART_FIRST_BIT_LSB;
    }

    if (rs485_config.start_bit_polarity == 1) {
        data.u32StartBitPolarity = USART_START_BIT_FALLING;
    } else {
        data.u32StartBitPolarity = USART_START_BIT_LOW;
    }

    if (rs485_config.hw_flow_control == 1) {
        data.u32HWFlowControl = USART_HW_FLOWCTRL_CTS;
    } else {
        data.u32HWFlowControl = USART_HW_FLOWCTRL_RTS; // HC32: RTS 等价于关闭硬件流控
    }
    return data;
}

/**
 * @brief 停止 TMR0 超时计数器
 */
static void usart_stop_timeout_timer(CM_TMR0_TypeDef *TMR0x, uint32_t u32Ch)
{
    uint32_t u32ClrMask;
    uint32_t u32SetMask;
    uint32_t u32BitOffset;

    u32BitOffset = 16UL * u32Ch;

    // 第一步: 配为同步时钟模式 (SYNCLKA=1, SYNSA=0)
    u32ClrMask = (TMR0_BCONR_SYNCLKA | TMR0_BCONR_SYNSA) << u32BitOffset;
    u32SetMask = TMR0_BCONR_SYNCLKA << u32BitOffset;
    MODIFY_REG32(TMR0x->BCONR, u32ClrMask, u32SetMask);

    // 第二步: 停止计数器 (CSTA=0, SYNCLKA=0, SYNSA=1)
    u32ClrMask = (TMR0_BCONR_SYNCLKA | TMR0_BCONR_SYNSA | TMR0_BCONR_CSTA) << u32BitOffset;
    u32SetMask = TMR0_BCONR_SYNSA << u32BitOffset;
    MODIFY_REG32(TMR0x->BCONR, u32ClrMask, u32SetMask);
}

/**
 * @brief USART 发送完成中断回调 (等 shift register 空了, 关 TX 切回 RS485 接收)
 */
static void rs485_tx_complete_irq_callback(void)
{
    USART_FuncCmd(RS485_UNIT, (USART_TX | USART_INT_TX_CPLT), DISABLE);
    RS485_CS_OFF();   // RS485 方向切回接收
}

/**
 * @brief USART 接收错误中断回调
 */
static void rs485_rx_error_irq_callback(void)
{
    (void)USART_ReadData(RS485_UNIT);

    USART_ClearStatus(RS485_UNIT, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

/**
 * @brief USART 接收超时中断回调 (判定不定长帧结束)
 */
static void rs485_rx_timeout_irq_callback(void)
{
    // 帧长 = 总配置长度 - DMA 剩余未传输数
    rs485_rx_frame_len = ARRAY_SZ(rs485_rx_buf) - (uint16_t)DMA_GetTransCount(RS485_RX_DMA_UNIT, RS485_RX_DMA_CH);

    // 停接收: 关 DMA 通道 + 关 USART RX, 防止新数据覆盖缓冲区
    DMA_ChCmd(RS485_RX_DMA_UNIT, RS485_RX_DMA_CH, DISABLE);
    USART_FuncCmd(RS485_UNIT, USART_RX, DISABLE);

    // 释放信号量, 通知操作系统有一帧数据到了
    if (rx_frame_done_sem != RT_NULL) {
        rt_sem_release(rx_frame_done_sem);
    }

    usart_stop_timeout_timer(TMR0_UNIT, TMR0_CH);
    USART_ClearStatus(RS485_UNIT, USART_FLAG_RX_TIMEOUT);
}

/**
 * @brief 初始化 RS485
 */
void rs485_init(void)
{
    // 创建接收完成信号量 (初始值 0, 中断释放, 线程等待)
    rx_frame_done_sem = rt_sem_create("rs485Rx", 0, RT_IPC_FLAG_FIFO);

    // 初始化 DMA
    (void)dma_init();

    // 初始化 TMR0 超时定时器
    tmr0_config(rs485_config.rs485_timeout_bits);

    // 解析配置参数, 初始化 USART
    stc_usart_uart_init_t data = rs485_data_init();
    GPIO_SetFunc(RS485_RX_PORT, RS485_RX_PIN, RS485_RX_FUNC);
    GPIO_SetFunc(RS485_TX_PORT, RS485_TX_PIN, RS485_TX_FUNC);
    RS485_FCG_ENABLE();
    data.u32ClockDiv  = USART_CLK_DIV64;
    data.u32CKOutput  = USART_CK_OUTPUT_ENABLE;
    data.u32Baudrate  = rs485_config.rs485_baudrate;
    data.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    if (LL_OK != USART_UART_Init(RS485_UNIT, &data, NULL)) {
        for (;;) {
        }
    }

    stc_irq_signin_config_t stcIrqSigninConfig;

    // 注册 USART 发送完成中断
    stcIrqSigninConfig.enIRQn      = RS485_TX_CPLT_IRQn;
    stcIrqSigninConfig.enIntSrc    = RS485_TX_CPLT_INT_SRC;
    stcIrqSigninConfig.pfnCallback = &rs485_tx_complete_irq_callback;
    (void)INTC_IrqSignIn(&stcIrqSigninConfig);
    NVIC_ClearPendingIRQ(stcIrqSigninConfig.enIRQn);
    NVIC_SetPriority(stcIrqSigninConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSigninConfig.enIRQn);

    // 注册 USART 接收错误中断
    stcIrqSigninConfig.enIRQn      = RS485_RX_ERR_IRQn;
    stcIrqSigninConfig.enIntSrc    = RS485_RX_ERR_INT_SRC;
    stcIrqSigninConfig.pfnCallback = &rs485_rx_error_irq_callback;
    (void)INTC_IrqSignIn(&stcIrqSigninConfig);
    NVIC_ClearPendingIRQ(stcIrqSigninConfig.enIRQn);
    NVIC_SetPriority(stcIrqSigninConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSigninConfig.enIRQn);

    // 注册 USART 接收超时中断
    stcIrqSigninConfig.enIRQn      = RS485_RX_TIMEOUT_IRQn;
    stcIrqSigninConfig.enIntSrc    = RS485_RX_TIMEOUT_INT_SRC;
    stcIrqSigninConfig.pfnCallback = &rs485_rx_timeout_irq_callback;
    (void)INTC_IrqSignIn(&stcIrqSigninConfig);
    NVIC_ClearPendingIRQ(stcIrqSigninConfig.enIRQn);
    NVIC_SetPriority(stcIrqSigninConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSigninConfig.enIRQn);

    // 初始化帧长
    rs485_rx_frame_len = 0U;

    // 使能 USART RX + 超时检测 + 相关中断
    USART_FuncCmd(RS485_UNIT, (USART_RX | USART_INT_RX | \
                               USART_RX_TIMEOUT | USART_INT_RX_TIMEOUT), ENABLE);
}
