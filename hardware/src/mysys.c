#include "mysys.h"

static int32_t bsp_xtal_init(void);

void bsp_clk_init(void) {
    stc_clock_pll_init_t stcPLLHInit;

    /* PCLK0, HCLK  Max 120MHz */
    /* PCLK1, PCLK2, PCLK3, PCLK4, EX BUS Max 60MHz */
    CLK_SetClockDiv(CLK_BUS_CLK_ALL,
                    (CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | CLK_PCLK2_DIV2 |
                     CLK_PCLK3_DIV2 | CLK_PCLK4_DIV2 | CLK_EXCLK_DIV2 |
                     CLK_HCLK_DIV1));

    (void)bsp_xtal_init();

    (void)CLK_PLLStructInit(&stcPLLHInit);
    /* VCO = (12/1)*40 = 480MHz*/
    stcPLLHInit.u8PLLState = CLK_PLL_ON;
    stcPLLHInit.PLLCFGR = 0UL;
    stcPLLHInit.PLLCFGR_f.PLLM = 1UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLN = 40UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLP = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLQ = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLR = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    (void)CLK_PLLInit(&stcPLLHInit);

    /* 2 cycles for 80 ~ 120MHz */
    (void)EFM_SetWaitCycle(EFM_WAIT_CYCLE2);
    /* 2 cycles for 100 ~ 120MHz */
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT2);
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);
    /* Reset cache ram */
    EFM_CacheRamReset(ENABLE);
    EFM_CacheRamReset(DISABLE);
    /* Enable cache */
    EFM_CacheCmd(EFM_CACHE_ALL, ENABLE);

    GPIO_SetDebugPort(GPIO_PIN_SWO, DISABLE);
    GPIO_SetDebugPort(GPIO_PIN_TDI,DISABLE);
    GPIO_SetDebugPort(GPIO_PIN_TRST,DISABLE);
}


static int32_t bsp_xtal_init(void) {
    stc_clock_xtal_init_t stcXtalInit;

    if (CLK_XTAL_ON == READ_REG8(CM_CMU->XTALCR)) {
        /* Disable xtal */
        (void)CLK_XtalCmd(DISABLE);
    }

    GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_PIN, ENABLE);

    (void)CLK_XtalStructInit(&stcXtalInit);
    /* Config Xtal and enable Xtal */
    stcXtalInit.u8Mode   = CLK_XTAL_MD_OSC;
    stcXtalInit.u8Drv    = CLK_XTAL_DRV_ULOW;
    stcXtalInit.u8State  = CLK_XTAL_ON;
    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    return CLK_XtalInit(&stcXtalInit);
}