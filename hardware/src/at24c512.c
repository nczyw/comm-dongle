#include "at24c512.h"

int32_t at24c512_write(
    uint16_t mem_addr, 
    const uint8_t *data_buf, 
    uint32_t data_buf_len
) {
    int32_t  i32_ret;
    uint32_t remain;
    uint32_t page_remain;
    uint32_t write_len;

    if (data_buf == NULL || data_buf_len == 0U) {
        return LL_ERR_INVD_PARAM;
    }
    if ((uint32_t)mem_addr + data_buf_len > AT24C512_CAPACITY) {
        return LL_ERR_INVD_PARAM;
    }
    remain = data_buf_len;
    while (remain > 0U) {
        page_remain = AT24C512_PAGE_SIZE - (mem_addr % AT24C512_PAGE_SIZE);
        write_len = (remain < page_remain) ? remain : page_remain;

        i32_ret = i2c1_write(AT24C512_DEV_ADDR, mem_addr, AT24C512_MEM_ADDR_LEN,
                             data_buf, write_len);
        if (i32_ret != LL_OK) {
            return i32_ret;   
        }
        mem_addr  += (uint16_t)write_len;
        data_buf  += write_len;
        remain    -= write_len;
    }
    return LL_OK;
}

int32_t at24c512_read(
    uint16_t mem_addr, 
    uint8_t *data_buf, 
    uint32_t data_buf_len
){
    if (data_buf == NULL || data_buf_len == 0) {
        return LL_ERR_INVD_PARAM;
    }

    if ((uint32_t) mem_addr + data_buf_len > AT24C512_CAPACITY) {
        return LL_ERR_INVD_PARAM;
    }
    return i2c1_read(AT24C512_DEV_ADDR, mem_addr, AT24C512_MEM_ADDR_LEN, data_buf, data_buf_len);
}
