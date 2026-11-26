#include "can.h"
#include "at24c512.h"

/*
const static char *m_s8ErrorTypeStr[] = {
    "NO error.",
    "Bit Error.",
    "Form Error.",
    "Stuff Error.",
    "ACK Error.",
    "CRC Error.",
    "Other Error.",
    "Error type is NOT defined.",
};
*/
static int32_t can_baud_check(
    const can_baud_cfg_t *pstcBaud,
    uint32_t u32Seg1Max, uint32_t u32Seg2Max,
    uint32_t u32SjwMax, uint32_t u32Gap
);
static int32_t can_clk_src_check(uint8_t u8ClkSrc);
static uint32_t can_id_type_to_ddl(uint8_t u8IdType);
static int32_t can_cfg_check(const can_eeprom_cfg_t *pstcCfg);
static void can_cfg_default(can_eeprom_cfg_t *pstcCfg);
static void can_clock_config(const can_eeprom_cfg_t *pstcCfg);
static void can_pin_config(void);
static void can_irq_config(void);


int32_t can_cfg_load(can_eeprom_cfg_t *pstcCfg)
{
    int32_t i32Ret;

    if (pstcCfg == NULL) {
        return LL_ERR_INVD_PARAM;
    }
    i32Ret = at24c512_read(CAN_EEPROM_BASE_ADDR, (uint8_t *)pstcCfg, sizeof(can_eeprom_cfg_t));
    if (i32Ret != LL_OK) {
        return i32Ret;
    }
    return can_cfg_check(pstcCfg);
}

int32_t can_cfg_save(const can_eeprom_cfg_t *pstcCfg)
{
    if (pstcCfg == NULL) {
        return LL_ERR_INVD_PARAM;
    }
    if (can_cfg_check(pstcCfg) != LL_OK) {
        return LL_ERR_INVD_PARAM;
    }
    return at24c512_wirte(CAN_EEPROM_BASE_ADDR, (const uint8_t *)pstcCfg, sizeof(can_eeprom_cfg_t));
}

void can_init_config(const can_eeprom_cfg_t *pstcCfg) {
    stc_can_init_t stc_can_init;        // 仲裁CAN配置
    stc_canfd_config_t stc_can_fd;      // CAN-FD 数据段配置
    stc_can_filter_config_t stc_can_filter[CAN_FILTER_MAX];       // 过滤器配置
    uint8_t i = 0U;

    if (can_cfg_check(pstcCfg) != LL_OK) {
        while(1);
    }

    (void)CAN_StructInit(&stc_can_init);
    // 仲裁段波特率（经典 CAN 与 CAN-FD 共用 SBT 寄存器）
    stc_can_init.stcBitCfg.u32Prescaler = pstcCfg->classic.prescaler;
    stc_can_init.stcBitCfg.u32TimeSeg1  = pstcCfg->classic.time_seg1;
    stc_can_init.stcBitCfg.u32TimeSeg2  = pstcCfg->classic.time_seg2;
    stc_can_init.stcBitCfg.u32SJW       = pstcCfg->classic.sjw;
    stc_can_init.u8WorkMode             = pstcCfg->work_mode;

    // CAN-FD 数据段波特率
    if (pstcCfg->fd.fd_enable != 0U) {
        (void)CAN_FD_StructInit(&stc_can_fd);
        stc_can_fd.stcBitCfg.u32Prescaler   = pstcCfg->fd.fast.prescaler;
        stc_can_fd.stcBitCfg.u32TimeSeg1    = pstcCfg->fd.fast.time_seg1;
        stc_can_fd.stcBitCfg.u32TimeSeg2    = pstcCfg->fd.fast.time_seg2;
        stc_can_fd.stcBitCfg.u32SJW         = pstcCfg->fd.fast.sjw;
        stc_can_fd.u8TDC                    = (pstcCfg->fd.tdc_enable != 0U) ? CAN_FD_TDC_ENABLE : CAN_FD_TDC_DISABLE;
        stc_can_fd.u8SSPOffset              = (uint8_t)pstcCfg->fd.fast.time_seg1;
        stc_can_fd.u8Mode                   = (pstcCfg->fd.fd_mode != 0U) ? CAN_FD_MD_ISO : CAN_FD_MD_BOSCH;
        stc_can_init.pstcCanFd = &stc_can_fd;
        CAN_FD_Cmd(CAN_UNIT, ENABLE);
    }

    // 过滤器：前 filter_count 个槽位连续启用 
    for (i = 0U; i < pstcCfg->filter_count; i++) {
        stc_can_filter[i].u32ID     = pstcCfg->filter[i].id;
        stc_can_filter[i].u32IDMask = pstcCfg->filter[i].mask;
        stc_can_filter[i].u32IDType = can_id_type_to_ddl(pstcCfg->filter[i].id_type);
    }
    if (pstcCfg->filter_count > 0U) {
        stc_can_init.pstcFilter      = stc_can_filter;
        stc_can_init.u16FilterSelect = (uint16_t)((1UL << pstcCfg->filter_count) - 1UL);
    } else {
        stc_can_filter[0].u32ID     = 0U;
        stc_can_filter[0].u32IDMask = 0x1FFFFFFFUL;
        stc_can_filter[0].u32IDType = CAN_ID_STD_EXT;
        stc_can_init.pstcFilter      = stc_can_filter;
        stc_can_init.u16FilterSelect = CAN_FILTER1;
    }

    FCG_Fcg1PeriphClockCmd(CAN_PERIPH_CLK, ENABLE);

    (void)CAN_Init(CAN_UNIT, &stc_can_init);
    CAN_IntCmd(CAN_UNIT, CAN_INT_ALL, DISABLE);
    CAN_IntCmd(CAN_UNIT, CAN_INT_SEL, ENABLE);
}
void can_init(void) {
    can_eeprom_cfg_t stcCfg;
    int32_t i32Ret;

    i32Ret = can_cfg_load(&stcCfg);
    if (i32Ret != LL_OK) {
        /* 新 EEPROM（全 0xFF）或数据损坏：使用默认配置 */
        can_cfg_default(&stcCfg);
    }
    can_clock_config(&stcCfg);
    can_pin_config();
    can_init_config(&stcCfg);
    can_irq_config();
}

// CAN 中断处理函数
void CAN_IRQ_HANDLER(void) {
    uint32_t u32Status;
    stc_can_error_info_t stcErr;

    u32Status = CAN_GetStatusValue(CAN_UNIT);
    if (u32Status != 0U) {
        CAN_ClearStatus(CAN_UNIT, u32Status);
    }

    if ((u32Status & CAN_FLAG_PTB_TX) != 0U) {
        //DDL_Printf("PTB transmitted.\r\n");
        //m_u8PTBTxFlag = 1U;
    }

    if ((u32Status & CAN_FLAG_RX) != 0U) {
        
        //DDL_Printf("Received a frame.\r\n");
        //m_u8RxFlag = 1U;
    }

    if ((u32Status & CAN_FLAG_ERR_INT) != 0U) {
        if ((u32Status & CAN_FLAG_BUS_OFF) != 0U) {
            //m_u8Busoff = 1U;
            //DDL_Printf("---> CAN BUS OFF\r\n");
        } else {
            (void)CAN_GetErrorInfo(CAN_UNIT, &stcErr);
            //DDL_Printf("---> CAN error type: %u, %s\r\n", stcErr.u8ErrorType, m_s8ErrorTypeStr[stcErr.u8ErrorType]);
        }
    }
    __DSB();  // Arm Errata 838869
}
/**
 * @brief  校验一组波特率参数，取值范围与驱动 IS_CAN_SBT / IS_CAN_FD_SBT / IS_CAN_FD_FBT 一致
 * @param  gap  TimeSeg1 至少比 TimeSeg2 大多少（经典/FD 仲裁段为 2，FD 数据段为 1）
 */
static int32_t can_baud_check(
    const can_baud_cfg_t *pstcBaud,
    uint32_t u32Seg1Max, uint32_t u32Seg2Max,
    uint32_t u32SjwMax, uint32_t u32Gap
) {
    if ((pstcBaud->prescaler < 1U) || (pstcBaud->prescaler > 256U)) {
        return LL_ERR_INVD_PARAM;
    }
    if ((pstcBaud->time_seg1 < 2U) || (pstcBaud->time_seg1 > u32Seg1Max)) {
        return LL_ERR_INVD_PARAM;
    }
    if ((pstcBaud->time_seg2 < 1U) || (pstcBaud->time_seg2 > u32Seg2Max)) {
        return LL_ERR_INVD_PARAM;
    }
    if ((pstcBaud->sjw < 1U) || (pstcBaud->sjw > u32SjwMax)) {
        return LL_ERR_INVD_PARAM;
    }
    if (pstcBaud->time_seg1 < (pstcBaud->time_seg2 + u32Gap)) {
        return LL_ERR_INVD_PARAM;
    }
    if (pstcBaud->time_seg2 < pstcBaud->sjw) {
        return LL_ERR_INVD_PARAM;
    }
    return LL_OK;
}

/* CAN 时钟源直接存 CLK_CANCLK_xxx 枚举值，这里只做合法性检查 */
static int32_t can_clk_src_check(uint8_t u8ClkSrc) {
    switch (u8ClkSrc) {
        case CLK_CANCLK_SYSCLK_DIV2:
        case CLK_CANCLK_SYSCLK_DIV3:
        case CLK_CANCLK_SYSCLK_DIV4:
        case CLK_CANCLK_SYSCLK_DIV5:
        case CLK_CANCLK_SYSCLK_DIV6:
        case CLK_CANCLK_SYSCLK_DIV7:
        case CLK_CANCLK_SYSCLK_DIV8:
        case CLK_CANCLK_PLLQ:
        case CLK_CANCLK_PLLR:
        case CLK_CANCLK_XTAL:
            return LL_OK;
        default:
            return LL_ERR_INVD_PARAM;
    }
}

/* EEPROM 中的 id_type(0=标准帧 1=扩展帧 2=双匹配) -> 驱动枚举 */
static uint32_t can_id_type_to_ddl(uint8_t u8IdType) {
    switch (u8IdType) {
        case 0U:
            return CAN_ID_STD;
        case 1U:
            return CAN_ID_EXT;
        default:
            return CAN_ID_STD_EXT;
    }
}

/* 完整校验一份配置，非法数据直接拒绝，避免带着脏数据去初始化硬件 */
static int32_t can_cfg_check(const can_eeprom_cfg_t *pstcCfg) {
    uint8_t u8FdEn = (pstcCfg->fd.fd_enable != 0U) ? 1U : 0U;
    uint8_t i;

    if (pstcCfg->fd.fd_enable > 1U) {
        return LL_ERR_INVD_PARAM;
    }
    if (pstcCfg->fd.fd_mode > 1U) {
        return LL_ERR_INVD_PARAM;
    }
    if (pstcCfg->fd.tdc_enable > 1U) {
        return LL_ERR_INVD_PARAM;
    }
    if (pstcCfg->work_mode > CAN_WORK_MD_ELB) {
        return LL_ERR_INVD_PARAM;
    }
    if (pstcCfg->filter_count > CAN_FILTER_MAX) {
        return LL_ERR_INVD_PARAM;
    }
    if (can_clk_src_check(pstcCfg->can_clk_src) != LL_OK) {
        return LL_ERR_INVD_PARAM;
    }

    /* FD 使能时驱动按 IS_CAN_FD_SBT 校验仲裁段，seg2/sjw 上限比纯经典模式宽 */
    if (can_baud_check(&pstcCfg->classic, 65U,
                       (u8FdEn != 0U) ? 32U : 8U,
                       (u8FdEn != 0U) ? 16U : 8U, 2U) != LL_OK) {
        return LL_ERR_INVD_PARAM;
    }
    if ((u8FdEn != 0U) && (can_baud_check(&pstcCfg->fd.fast, 17U, 8U, 8U, 1U) != LL_OK)) {
        return LL_ERR_INVD_PARAM;
    }

    for (i = 0U; i < pstcCfg->filter_count; i++) {
        if (pstcCfg->filter[i].id_type > 2U) {
            return LL_ERR_INVD_PARAM;
        }
        /* 标准帧 ID 只有 11 bit，扩展帧 ID 只有 29 bit */
        if ((pstcCfg->filter[i].id_type != 1U) && (pstcCfg->filter[i].id > 0x7FFUL)) {
            return LL_ERR_INVD_PARAM;
        }
        if ((pstcCfg->filter[i].id_type != 0U) && (pstcCfg->filter[i].id > 0x1FFFFFFFUL)) {
            return LL_ERR_INVD_PARAM;
        }
        if (pstcCfg->filter[i].mask > 0x1FFFFFFFUL) {
            return LL_ERR_INVD_PARAM;
        }
    }
    return LL_OK;
}

/* 全新 EEPROM（全 0xFF）或数据损坏时的默认配置
   CAN 时钟 = SYSCLK 120MHz / 2 = 60MHz
   仲裁段 1Mbps：60MHz / (2 × (24 + 6))，采样点 80%
   FD 数据段 5Mbps：60MHz / (1 × (9 + 3))，采样点 75% */
static void can_cfg_default(can_eeprom_cfg_t *pstcCfg) {
    *pstcCfg = (can_eeprom_cfg_t){0};

    pstcCfg->can_clk_src  = CLK_CANCLK_SYSCLK_DIV2;
    pstcCfg->work_mode    = CAN_WORK_MD_NORMAL;

    /* 默认不过滤：mask 全 1 表示 ID 所有位都不参与比较，标准帧/扩展帧都接收 */
    pstcCfg->filter_count      = 1U;
    pstcCfg->filter[0].id       = 0U;
    pstcCfg->filter[0].mask     = 0x1FFFFFFFUL;
    pstcCfg->filter[0].id_type  = 2U;

    pstcCfg->classic.prescaler = 2U;
    pstcCfg->classic.time_seg1 = 24U;
    pstcCfg->classic.time_seg2 = 6U;
    pstcCfg->classic.sjw       = 4U;

    pstcCfg->fd.fd_enable      = 1U;
    pstcCfg->fd.fd_mode        = 1U;    /* ISO CAN FD */
    pstcCfg->fd.tdc_enable     = 1U;
    pstcCfg->fd.fast.prescaler = 1U;
    pstcCfg->fd.fast.time_seg1 = 9U;
    pstcCfg->fd.fast.time_seg2 = 3U;
    pstcCfg->fd.fast.sjw       = 3U;
}

static void can_clock_config(const can_eeprom_cfg_t *pstcCfg) {
    CLK_SetCANClockSrc(CAN_CLK_UNIT, pstcCfg->can_clk_src);
}

static void can_pin_config(void) {
    GPIO_SetFunc(CAN_TX_PORT, CAN_TX_PIN, CAN_TX_PIN_FUNC);
    GPIO_SetFunc(CAN_RX_PORT, CAN_RX_PIN, CAN_RX_PIN_FUNC);
}

static void can_irq_config(void) {
    //INTC_IrqSignIn()
    NVIC_ClearPendingIRQ(CAN_INT_IRQn);
    NVIC_SetPriority(CAN_INT_IRQn, CAN_INT_PRIO);
    NVIC_EnableIRQ(CAN_INT_IRQn);
}