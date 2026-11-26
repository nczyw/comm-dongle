#ifndef I2C_H
#define I2C_H

#include "hc32_ll.h"

#if defined(RTTHREAD)
#include "rtthread.h"
extern rt_mutex_t i2c1_mutex;
#endif

// I2C 单元
#define I2C1_UNIT                (CM_I2C1)
#define I2C1_FCB                 (FCG1_PERIPH_I2C1)

// I2C SCL引脚定义
#define I2C1_SCL_PORT           (GPIO_PORT_A)
#define I2C1_SCL_PIN            (GPIO_PIN_05)
#define I2C1_SCL_FUNC           (GPIO_FUNC_55)

// I2C SDA引脚定义
#define I2C1_SDA_PORT           (GPIO_PORT_A)
#define I2C1_SDA_PIN            (GPIO_PIN_04)
#define I2C1_SDA_FUNC           (GPIO_FUNC_54)

#define I2C1_BAUDRATE            (400000UL)
#define I2C1_TIMEOUT             (0x400U)

int32_t i2c1_init(void);
void i2c1_deinit(void);

int32_t i2c1_write(
    uint16_t        dev_addr,
    uint16_t        mem_addr,
    uint8_t         mem_addr_len,
    const uint8_t   *data_buf,
    uint32_t        data_buf_len
);

int32_t i2c1_read(
    uint16_t        dev_addr,
    uint16_t        mem_addr,
    uint8_t         mem_addr_len,
    uint8_t         *data_buf,
    uint32_t        data_buf_len
);

#endif
