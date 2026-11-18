#include "rs485.h"

uint8_t rs485_rx_buf[RS485_RX_BUF_LEN];
stc_rs485_config_t rs485_config;


static int32_t dma_init(void);
static void TMR0_Config(uint16_t timeout_bits);
static stc_usart_uart_init_t rs485_data_init(void);
static void RS485_TxComplete_IrqCallback(void);
static void RS485_RxError_IrqCallback(void);
static void RS485_RxTimeout_IrqCallback(void);
static void RS485_RX_DMA_TC_IrqCallback(void);

static int32_t dma_init(void) {
    int32_t i32Ret;
    stc_dma_init_t stcDmaInit;
    stc_dma_llp_init_t stcDmaLlpInit;
    stc_irq_signin_config_t stcIrqSignConfig;
    static stc_dma_llp_descriptor_t stcLlpDesc;

    /* DMA&AOS FCG enable */
    RS485_RX_DMA_FCG_ENABLE();

    // USART_RX_DMA
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn = DMA_INT_ENABLE;
    stcDmaInit.u32BlockSize = 1UL;
    stcDmaInit.u32TransCount = ARRAY_SZ(rs485_rx_buf);
    stcDmaInit.u32DataWidth = DMA_DATAWIDTH_8BIT;
    stcDmaInit.u32DestAddr = (uint32_t)rs485_rx_buf;
    stcDmaInit.u32SrcAddr = (uint32_t)(&RS485_UNIT->RDR);
    stcDmaInit.u32SrcAddrInc = DMA_SRC_ADDR_FIX;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_INC;
    i32Ret = DMA_Init(RS485_RX_DMA_UNIT, RS485_RX_DMA_CH, &stcDmaInit);
    if (LL_OK == i32Ret) {
        (void)DMA_LlpStructInit(&stcDmaLlpInit);
        stcDmaLlpInit.u32State = DMA_LLP_ENABLE;
        stcDmaLlpInit.u32Mode  = DMA_LLP_WAIT;
        stcDmaLlpInit.u32Addr  = (uint32_t)&stcLlpDesc;
        (void)DMA_LlpInit(RS485_RX_DMA_UNIT, RS485_RX_DMA_CH, &stcDmaLlpInit);

        stcLlpDesc.SARx   = stcDmaInit.u32SrcAddr;
        stcLlpDesc.DARx   = stcDmaInit.u32DestAddr;
        stcLlpDesc.DTCTLx = (stcDmaInit.u32TransCount << DMA_DTCTL_CNT_POS) | (stcDmaInit.u32BlockSize << DMA_DTCTL_BLKSIZE_POS);;
        stcLlpDesc.LLPx   = (uint32_t)&stcLlpDesc;
        stcLlpDesc.CHCTLx = stcDmaInit.u32SrcAddrInc | stcDmaInit.u32DestAddrInc | stcDmaInit.u32DataWidth |  \
                            stcDmaInit.u32IntEn      | stcDmaLlpInit.u32State    | stcDmaLlpInit.u32Mode;

        DMA_ReconfigLlpCmd(RS485_RX_DMA_UNIT, RS485_RX_DMA_CH, ENABLE);
        DMA_ReconfigCmd(RS485_RX_DMA_UNIT, ENABLE);

        stcIrqSignConfig.enIntSrc = RS485_RX_DMA_TC_INT_SRC;
        stcIrqSignConfig.enIRQn  = RS485_RX_DMA_TC_IRQn;
        stcIrqSignConfig.pfnCallback = &RS485_RX_DMA_TC_IrqCallback;
        (void)INTC_IrqSignIn(&stcIrqSignConfig);
        NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
        NVIC_SetPriority(stcIrqSignConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
        NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);

        DMA_Cmd(RS485_RX_DMA_UNIT, ENABLE);
        DMA_TransCompleteIntCmd(RS485_RX_DMA_UNIT, RS485_RX_DMA_TC_INT, ENABLE);
        (void)DMA_ChCmd(RS485_RX_DMA_UNIT, RS485_RX_DMA_CH, ENABLE);
    }
}

static void TMR0_Config(uint16_t timeout_bits) {
    uint16_t u16Div;
    uint16_t u16Delay;
    uint16_t u16CompareValue;
    stc_tmr0_init_t stcTmr0Init;

    TMR0_FCG_ENABLE();

    /* Initialize TMR0 base function. */
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

    TMR0_HWStartCondCmd(TMR0_UNIT, TMR0_CH, ENABLE);
    TMR0_HWClearCondCmd(TMR0_UNIT, TMR0_CH, ENABLE);
}

static stc_usart_uart_init_t rs485_data_init(void) {
    stc_usart_uart_init_t data ;
    data.u32Baudrate = rs485_config.rs485_baudrate;

    if(rs485_config.data_width == 1){
        data.u32DataWidth = USART_DATA_WIDTH_9BIT;
    }else{
        data.u32DataWidth = USART_DATA_WIDTH_8BIT;
    }

    if(rs485_config.stop_bits == 1){
        data.u32StopBit = USART_STOPBIT_2BIT;
    }else{
        data.u32StopBit = USART_STOPBIT_1BIT;
    }

    if(rs485_config.parity == 1){
        data.u32Parity = USART_PARITY_EVEN;
    }else if(rs485_config.parity == 2){
        data.u32Parity = USART_PARITY_ODD;
    }else{
        data.u32Parity = USART_PARITY_NONE;
    }

    if(rs485_config.over_sample_bit == 1){
        data.u32OverSampleBit = USART_OVER_SAMPLE_16BIT;
    }else{
        data.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    }

    if(rs485_config.first_bit == 1){
        data.u32FirstBit = USART_FIRST_BIT_MSB;
    }else{
        data.u32FirstBit = USART_FIRST_BIT_LSB;
    }

    if(rs485_config.start_bit_polarity == 1){
        data.u32StartBitPolarity = USART_START_BIT_FALLING;
    }else{
        data.u32StartBitPolarity = USART_START_BIT_LOW;
    }
    if(rs485_config.hw_flow_control == 1){
        data.u32HWFlowControl = USART_HW_FLOWCTRL_CTS;
    } else {
        data.u32HWFlowControl = USART_HW_FLOWCTRL_RTS;
    }
    return data;
}

// 发送完成中断回调函数
static void RS485_TxComplete_IrqCallback(void) {
    USART_FuncCmd(RS485_UNIT, (USART_TX | USART_INT_TX_CPLT), DISABLE);
}
// 接收错误中断回调函数
static void RS485_RxError_IrqCallback(void) {
    (void)USART_ReadData(RS485_UNIT);
    USART_ClearStatus(RS485_UNIT, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}
// 接收超时中断回调函数
static void RS485_RxTimeout_IrqCallback(void) {
    USART_StopTimeoutTimer(TMR0_UNIT, TMR0_CH);
    USART_ClearStatus(RS485_UNIT, USART_FLAG_RX_TIMEOUT);
}

// 接收DMA传输完成中断回调函数
static void RS485_RX_DMA_TC_IrqCallback(void) {
    USART_FuncCmd(RS485_UNIT, USART_RX_TIMEOUT, DISABLE);
    DMA_ClearTransCompleteStatus(RS485_RX_DMA_UNIT, RS485_RX_DMA_TC_FLAG);
}

/**
 * @brief 初始化 RS485
 */
void rs485_init(void){
    // 初始化 TMR0
    TMR0_Config(rs485_config.rs485_timeout_bits);
    // 解析配置参数
    stc_usart_uart_init_t data = rs485_data_init();
    GPIO_SetFunc(RS485_RX_PORT, RS485_RX_PIN, RS485_RX_FUNC);
    GPIO_SetFunc(RS485_TX_PORT, RS485_TX_PIN, RS485_TX_FUNC);
    RS485_FCG_ENABLE();
    data.u32ClockDiv = USART_CLK_DIV64;
    data.u32CKOutput = USART_CK_OUTPUT_ENABLE;
    if (LL_OK != USART_UART_Init(RS485_UNIT, &data, NULL)) {
        for (;;) {
        }
    }

    stc_irq_signin_config_t stcIrqSigninConfig;
    
    // Register TX complete IRQ handler.
    stcIrqSigninConfig.enIRQn = RS485_TX_CPLT_IRQn;
    stcIrqSigninConfig.enIntSrc = RS485_TX_CPLT_INT_SRC;
    stcIrqSigninConfig.pfnCallback = &RS485_TxComplete_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSigninConfig);
    NVIC_ClearPendingIRQ(stcIrqSigninConfig.enIRQn);
    NVIC_SetPriority(stcIrqSigninConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSigninConfig.enIRQn);

    // Register RX error IRQ handler. 
    stcIrqSigninConfig.enIRQn = RS485_RX_ERR_IRQn;
    stcIrqSigninConfig.enIntSrc = RS485_RX_ERR_INT_SRC;
    stcIrqSigninConfig.pfnCallback = &RS485_RxError_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSigninConfig);
    NVIC_ClearPendingIRQ(stcIrqSigninConfig.enIRQn);
    NVIC_SetPriority(stcIrqSigninConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSigninConfig.enIRQn);

    // Register RX timeout IRQ handler.
    stcIrqSigninConfig.enIRQn = RS485_RX_TIMEOUT_IRQn;
    stcIrqSigninConfig.enIntSrc = RS485_RX_TIMEOUT_INT_SRC;
    stcIrqSigninConfig.pfnCallback = &RS485_RxTimeout_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSigninConfig);
    NVIC_ClearPendingIRQ(stcIrqSigninConfig.enIRQn);
    NVIC_SetPriority(stcIrqSigninConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSigninConfig.enIRQn);
}
