#include "i2c.h"

rt_mutex_t i2c1_mutex = RT_NULL;
rt_mutex_t i2c3_mutex = RT_NULL;

static int32_t BSP_I2C_Init(CM_I2C_TypeDef *I2Cx , uint32_t baudrate);

int32_t i2c1_init(void) {
    stc_gpio_init_t stcGpioInit;
    /* Configuration I2C GPIO */
    (void)GPIO_StructInit(&stcGpioInit);
    (void)GPIO_Init(I2C1_SCL_PORT, I2C1_SCL_PIN, &stcGpioInit);
    (void)GPIO_Init(I2C1_SDA_PORT, I2C1_SDA_PIN, &stcGpioInit);
    GPIO_SetFunc(I2C1_SCL_PORT, I2C1_SCL_PIN, I2C1_SCL_FUNC);
    GPIO_SetFunc(I2C1_SDA_PORT, I2C1_SDA_PIN, I2C1_SDA_FUNC);
    /* Enable I2C Peripheral*/
    FCG_Fcg1PeriphClockCmd(I2C1_FCG, ENABLE);

    i2c1_mutex = rt_mutex_create("i2c1_mutex", RT_IPC_FLAG_FIFO);
    if (i2c1_mutex == RT_NULL) {
        return LL_ERR_INVD_PARAM;
    }
    return BSP_I2C_Init(I2C1_UNIT,I2C1_BAUDRATE);
}

void i2c1_deint(void) {
    GPIO_SetFunc(I2C1_SCL_PORT, I2C1_SCL_PIN, GPIO_FUNC_0);
    GPIO_SetFunc(I2C1_SDA_PORT, I2C1_SDA_PIN, GPIO_FUNC_0);
    (void)I2C_DeInit(I2C1_UNIT);
}

int32_t i2c3_init(void) {
    stc_gpio_init_t stcGpioInit;
    /* Configuration I2C GPIO */
    (void)GPIO_StructInit(&stcGpioInit);
    (void)GPIO_Init(I2C3_SCL_PORT, I2C3_SCL_PIN, &stcGpioInit);
    (void)GPIO_Init(I2C3_SDA_PORT, I2C3_SDA_PIN, &stcGpioInit);
    GPIO_SetFunc(I2C3_SCL_PORT, I2C3_SCL_PIN, I2C3_SCL_FUNC);
    GPIO_SetFunc(I2C3_SDA_PORT, I2C3_SDA_PIN, I2C3_SDA_FUNC);
    /* Enable I2C Peripheral*/
    FCG_Fcg1PeriphClockCmd(I2C3_FCG, ENABLE);

    i2c3_mutex = rt_mutex_create("i2c3_mutex", RT_IPC_FLAG_FIFO);
    if (i2c3_mutex == RT_NULL) {
        return LL_ERR_INVD_PARAM;
    }
    return BSP_I2C_Init(I2C3_UNIT,I2C3_BAUDRATE);
}

void i2c3_deint(void) {
    GPIO_SetFunc(I2C3_SCL_PORT, I2C3_SCL_PIN, GPIO_FUNC_0);
    GPIO_SetFunc(I2C3_SDA_PORT, I2C3_SDA_PIN, GPIO_FUNC_0);
    (void)I2C_DeInit(I2C3_UNIT);
}
int32_t i2c1_write(
    uint8_t dev_addr,
    uint16_t mem_addr, 
    uint8_t mem_addr_len, 
    const uint8_t *data_buf, 
    uint16_t data_buf_len
) {
    int32_t i32_ret = LL_ERR_BUSY;
    uint16_t mem_addr_len_temp = 0;
    if (mem_addr_len == 1) {
        mem_addr_len_temp = mem_addr;
    } else {
        mem_addr_len_temp = (uint16_t)((((uint32_t)mem_addr >> 8) & 0xffUL) + (((uint32_t)mem_addr << 8) & 0xff00UL));
    }

    rt_mutex_take(i2c1_mutex, RT_WAITING_FOREVER);

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
    rt_mutex_release(i2c1_mutex);
    return i32_ret;
}
int32_t i2c1_read(
    uint8_t dev_addr, 
    uint16_t mem_addr, 
    uint8_t mem_addr_len, 
    uint8_t *data_buf, 
    uint16_t data_buf_len
) {
    int32_t i32_ret = LL_ERR_BUSY;
    uint16_t mem_addr_len_temp = 0;
    if (mem_addr_len == 1) {
        mem_addr_len_temp = mem_addr;
    } else {
        mem_addr_len_temp = (uint16_t)((((uint32_t)mem_addr >> 8) & 0xffUL) + (((uint32_t)mem_addr << 8) & 0xff00UL));
    }
    rt_mutex_take(i2c1_mutex, RT_WAITING_FOREVER);
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
    rt_mutex_release(i2c1_mutex);
    return i32_ret;
}
int32_t i2c3_write(
    uint8_t dev_addr, 
    uint16_t mem_addr, 
    uint8_t mem_addr_len, 
    const uint8_t *data_buf, 
    uint16_t data_buf_len
) {
    int32_t i32_ret = LL_ERR_BUSY;
    uint16_t mem_addr_len_temp = 0;
    if (mem_addr_len == 1) {
        mem_addr_len_temp = mem_addr;
    } else {
        mem_addr_len_temp = (uint16_t)((((uint32_t)mem_addr >> 8) & 0xffUL) + (((uint32_t)mem_addr << 8) & 0xff00UL));
    }

    rt_mutex_take(i2c3_mutex, RT_WAITING_FOREVER);

    I2C_SWResetCmd(I2C3_UNIT, ENABLE);
    I2C_SWResetCmd(I2C3_UNIT, DISABLE);

    i32_ret = I2C_Start(I2C3_UNIT, I2C3_TIMEOUT);
    if (i32_ret == LL_OK) {
        i32_ret = I2C_TransAddr(I2C3_UNIT, dev_addr, I2C_DIR_TX, I2C3_TIMEOUT);
        if (i32_ret == LL_OK) {
            i32_ret = I2C_TransData(I2C3_UNIT, (const uint8_t *)&mem_addr_len_temp, mem_addr_len, I2C3_TIMEOUT);
            if (i32_ret == LL_OK) {
                i32_ret = I2C_TransData(I2C3_UNIT, data_buf, data_buf_len, I2C3_TIMEOUT);
            }
        }
    }
    (void)I2C_Stop(I2C3_UNIT, I2C3_TIMEOUT);
    rt_mutex_release(i2c3_mutex);
    return i32_ret;
}
int32_t i2c3_read(
    uint8_t dev_addr, 
    uint16_t mem_addr, 
    uint8_t mem_addr_len, 
    uint8_t *data_buf, 
    uint16_t data_buf_len
){
    int32_t i32_ret = LL_ERR_BUSY;
    uint16_t mem_addr_len_temp = 0;
    if (mem_addr_len == 1) {
        mem_addr_len_temp = mem_addr;
    } else {
        mem_addr_len_temp = (uint16_t)((((uint32_t)mem_addr >> 8) & 0xffUL) + (((uint32_t)mem_addr << 8) & 0xff00UL));
    }
    rt_mutex_take(i2c3_mutex, RT_WAITING_FOREVER);
    I2C_SWResetCmd(I2C3_UNIT, ENABLE);
    I2C_SWResetCmd(I2C3_UNIT, DISABLE);
    i32_ret = I2C_Start(I2C3_UNIT, I2C3_TIMEOUT);
    if (i32_ret == LL_OK) {
        i32_ret = I2C_TransAddr(I2C3_UNIT, dev_addr, I2C_DIR_TX, I2C3_TIMEOUT);
        if (i32_ret == LL_OK) {
            i32_ret = I2C_TransData(I2C3_UNIT, (const uint8_t *)&mem_addr_len_temp, mem_addr_len, I2C3_TIMEOUT);
            if (i32_ret == LL_OK) {
                i32_ret = I2C_Restart(I2C3_UNIT, I2C3_TIMEOUT);
                if (i32_ret == LL_OK) {
                    if (data_buf_len == 1UL) {
                        I2C_AckConfig(I2C3_UNIT, I2C_NACK);
                    }
                    i32_ret = I2C_TransAddr(I2C3_UNIT, dev_addr, I2C_DIR_RX, I2C3_TIMEOUT);
                    if (i32_ret == LL_OK) {
                        i32_ret = I2C_MasterReceiveDataAndStop(I2C3_UNIT, data_buf, data_buf_len, I2C3_TIMEOUT);
                    }
                    I2C_AckConfig(I2C3_UNIT, I2C_ACK);
                }
            }
        }
    }
    if (i32_ret != LL_OK) {
        (void)I2C_Stop(I2C3_UNIT, I2C3_TIMEOUT);
    }
    rt_mutex_release(i2c3_mutex);
    return i32_ret;
}
/**
 * @brief  BSP I2C initialize
 * @param  [in] I2Cx                Pointer to I2C instance register base.
 *                                  This parameter can be a value of the following:
 *         @arg CM_I2Cx:            I2C instance register base.
 * @retval int32_t:
 *            - LL_OK:              Configure success
 *            - LL_ERR_INVD_PARAM:  Invalid parameter
 */
static int32_t BSP_I2C_Init(CM_I2C_TypeDef *I2Cx , uint32_t baudrate)
{
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