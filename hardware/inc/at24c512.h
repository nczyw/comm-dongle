#ifndef AT24C512_H
#define AT24C512_H 

#include "i2c.h"

#define AT24C512_DEV_ADDR       (0xA2U >> 1)
#define AT24C512_MEM_ADDR_LEN   (2U)
#define AT24C512_PAGE_SIZE      (128U)
#define AT24C512_CAPACITY       (65536U)

int32_t at24c512_write(
    uint16_t        mem_addr,
    const uint8_t * data_buf,
    uint32_t        data_buf_len
);

int32_t at24c512_read(
    uint16_t        mem_addr,
    uint8_t *       data_buf,
    uint32_t        data_buf_len
);


#endif