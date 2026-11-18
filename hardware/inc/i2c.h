#ifndef I2C_H
#define I2C_H

#include "hc32_ll.h"
#include "rtthread.h"

#define I2C1_UNIT           CM_I2C1
#define I2C1_FCG            (FCG1_PERIPH_I2C1)

#define I2C1_SCL_PORT       GPIO_PORT_B
#define I2C1_SCL_PIN        GPIO_PIN_12
#define I2C1_SCL_FUNC       GPIO_FUNC_49

#define I2C1_SDA_PORT       GPIO_PORT_B
#define I2C1_SDA_PIN        GPIO_PIN_13
#define I2C1_SDA_FUNC       GPIO_FUNC_48

#define I2C1_BAUDRATE       (400000UL)
#define I2C1_TIMEOUT        (0x400U)


#define I2C3_UNIT           CM_I2C3
#define I2C3_FCG            (FCG1_PERIPH_I2C3)

#define I2C3_SCL_PORT       GPIO_PORT_B
#define I2C3_SCL_PIN        GPIO_PIN_12
#define I2C3_SCL_FUNC       GPIO_FUNC_49

#define I2C3_SDA_PORT       GPIO_PORT_B
#define I2C3_SDA_PIN        GPIO_PIN_13
#define I2C3_SDA_FUNC       GPIO_FUNC_48

#define I2C3_BAUDRATE       (400000UL)
#define I2C3_TIMEOUT        (0x400U)

extern rt_mutex_t i2c1_mutex;
extern rt_mutex_t i2c3_mutex;

int32_t i2c1_init(void);
void i2c1_deint(void);

int32_t i2c3_init(void);
void i2c3_deint(void);

int32_t i2c1_write(
    uint8_t             dev_addr,
    uint16_t            mem_addr,
    uint8_t             mem_addr_len,
    const uint8_t       * data_buf,
    uint16_t            data_buf_len
);

int32_t i2c1_read(
    uint8_t             dev_addr,
    uint16_t            mem_addr,
    uint8_t             mem_addr_len,
    uint8_t             * data_buf,
    uint16_t            data_buf_len
);

int32_t i2c3_write(
    uint8_t             dev_addr,
    uint16_t            mem_addr,
    uint8_t             mem_addr_len,
    const uint8_t       * data_buf,
    uint16_t            data_buf_len
);

int32_t i2c3_read(
    uint8_t             dev_addr,
    uint16_t            mem_addr,
    uint8_t             mem_addr_len,
    uint8_t             * data_buf,
    uint16_t            data_buf_len
);

#endif