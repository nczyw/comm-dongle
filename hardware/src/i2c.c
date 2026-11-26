#include "i2c.h"

#if defined(RTTHREAD)
rt_mutex_t i2c1_mutex = RT_NULL;
#endif

static int32_t BSP_I2C_Init(CM_I2C_TypeDef *I2Cx , uint32_t baudrate);

int32_t i2c1_init(void) {
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
    (void)GPIO_Init(I2C1_SCL_PORT, I2C1_SCL_PIN, &stcGpioInit);
    (void)GPIO_Init(I2C1_SDA_PORT, I2C1_SDA_PIN, &stcGpioInit);
    GPIO_SetFunc(I2C1_SCL_PORT, I2C1_SCL_PIN, I2C1_SCL_FUNC);
    GPIO_SetFunc(I2C1_SDA_PORT, I2C1_SDA_PIN, I2C1_SDA_FUNC);
    /* Enable I2C Peripheral*/
    FCG_Fcg1PeriphClockCmd(I2C1_FCB, ENABLE);
#if defined(RTTHREAD)
    i2c1_mutex = rt_mutex_create("i2c1_mutex", RT_IPC_FLAG_FIFO);
    if( i2c1_mutex == RT_NULL) {
        rt_kprintf("i2c1_mutex create failed!\n");
        while(1);
    }
#endif
    return BSP_I2C_Init(I2C1_UNIT,I2C1_BAUDRATE);
}

void i2c1_deinit(void){
#if defined(RTTHREAD)
    rt_mutex_delete(i2c1_mutex);
#endif
    GPIO_SetFunc(I2C1_SCL_PORT, I2C1_SCL_PIN, GPIO_FUNC_0);
    GPIO_SetFunc(I2C1_SDA_PORT, I2C1_SDA_PIN, GPIO_FUNC_0);
    (void)I2C_DeInit(I2C1_UNIT);
}

int32_t i2c1_write(
    uint16_t        dev_addr,
    uint16_t        mem_addr,
    uint8_t         mem_addr_len,
    const uint8_t   *data_buf,
    uint32_t        data_buf_len
) {
    int32_t i32_ret = LL_ERR_BUSY;
    uint16_t mem_addr_len_temp = 0;
#if defined(RTTHREAD)
    rt_mutex_take(i2c1_mutex, RT_WAITING_FOREVER);
#endif
    if (mem_addr_len == 1) {
        mem_addr_len_temp = mem_addr;
    } else {
        mem_addr_len_temp = (uint16_t)((((uint32_t)mem_addr >> 8) & 0xffUL) + (((uint32_t)mem_addr << 8) & 0xff00UL));
    }

    I2C_SWResetCmd(I2C1_UNIT, ENABLE);
    I2C_SWResetCmd(I2C1_UNIT, DISABLE);
    i32_ret = I2C_Start(I2C1_UNIT, I2C1_TIMEOUT);

    if (i32_ret == LL_OK) {
        i32_ret = I2C_TransAddr(I2C1_UNIT, dev_addr, I2C_DIR_TX, I2C1_TIMEOUT);
        if (i32_ret == LL_OK) {
            i32_ret = I2C_TransData(I2C1_UNIT, (const uint8_t *)&mem_addr_len_temp, mem_addr_len, I2C1_TIMEOUT);
            if (i32_ret == LL_OK) {
                i32_ret = I2C_TransData(I2C1_UNIT, data_buf, data_buf_len, I2C1_TIMEOUT);
            }
        }
    }
    (void)I2C_Stop(I2C1_UNIT, I2C1_TIMEOUT);
#if defined(RTTHREAD)
        rt_mutex_release(i2c1_mutex);
#endif
    return i32_ret;
}


int32_t i2c1_read(
    uint16_t        dev_addr,
    uint16_t        mem_addr,
    uint8_t         mem_addr_len,
    uint8_t         *data_buf,
    uint32_t        data_buf_len
) {
    int32_t i32_ret = LL_ERR_BUSY;
    uint16_t mem_addr_len_temp = 0;
#if defined(RTTHREAD)
        rt_mutex_take(i2c1_mutex, RT_WAITING_FOREVER);
#endif
    if (mem_addr_len == 1) {
        mem_addr_len_temp = mem_addr;
    } else {
        mem_addr_len_temp = (uint16_t)((((uint32_t)mem_addr >> 8) & 0xffUL) + (((uint32_t)mem_addr << 8) & 0xff00UL));
    }
    I2C_SWResetCmd(I2C1_UNIT, ENABLE);
    I2C_SWResetCmd(I2C1_UNIT, DISABLE);
    i32_ret = I2C_Start(I2C1_UNIT, I2C1_TIMEOUT);
    if (i32_ret == LL_OK) {
        i32_ret = I2C_TransAddr(I2C1_UNIT, dev_addr, I2C_DIR_TX, I2C1_TIMEOUT);
        if (i32_ret == LL_OK) {
            i32_ret = I2C_TransData(I2C1_UNIT, (const uint8_t *)&mem_addr_len_temp, mem_addr_len, I2C1_TIMEOUT);
            if (i32_ret == LL_OK) {
                i32_ret = I2C_Restart(I2C1_UNIT, I2C1_TIMEOUT);
                if (i32_ret == LL_OK) {
                    if (data_buf_len == 1UL) {
                        I2C_AckConfig(I2C1_UNIT, I2C_NACK);
                    }
                    i32_ret = I2C_TransAddr(I2C1_UNIT, dev_addr, I2C_DIR_RX, I2C1_TIMEOUT);
                    if (i32_ret == LL_OK) {
                        i32_ret = I2C_MasterReceiveDataAndStop(I2C1_UNIT, data_buf, data_buf_len, I2C1_TIMEOUT);
                    }
                    I2C_AckConfig(I2C1_UNIT, I2C_ACK);
                }
            }
        }
    }
    if (i32_ret != LL_OK) {
        (void)I2C_Stop(I2C1_UNIT, I2C1_TIMEOUT);
    }
#if defined(RTTHREAD)
        rt_mutex_release(i2c1_mutex);
#endif
    return i32_ret;
}
static int32_t BSP_I2C_Init(CM_I2C_TypeDef *I2Cx , uint32_t baudrate) {
    int32_t i32Ret;
    float32_t fErr;
    stc_i2c_init_t stcI2cInit;
    uint32_t I2cSrcClk;
    uint32_t I2cClkDiv;
    uint32_t I2cClkDivReg;

    I2cSrcClk = I2C_SRC_CLK;
    I2cClkDiv = I2cSrcClk / baudrate / I2C_WIDTH_MAX_IMME;
    for (I2cClkDivReg = I2C_CLK_DIV1; I2cClkDivReg <= I2C_CLK_DIV128; I2cClkDivReg++) {
        if (I2cClkDiv < (1UL << I2cClkDivReg)) {
            break;
        }
    }

    (void)I2C_DeInit(I2Cx);
    (void)I2C_StructInit(&stcI2cInit);
    stcI2cInit.u32Baudrate = baudrate;
    stcI2cInit.u32SclTime  = (uint32_t)((uint64_t)400UL * ((uint64_t)I2cSrcClk / ((uint64_t)1UL << I2cClkDivReg)) / (uint64_t)1000000000UL);  /* SCL time is about 400nS in EVB board */
    stcI2cInit.u32ClockDiv = I2cClkDivReg;
    i32Ret = I2C_Init(I2Cx, &stcI2cInit, &fErr);

    if (LL_OK == i32Ret) {
        I2C_BusWaitCmd(I2Cx, ENABLE);
        I2C_Cmd(I2Cx, ENABLE);
    }

    return i32Ret;
}