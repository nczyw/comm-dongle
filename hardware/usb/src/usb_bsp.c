// USB BSP：USB 引脚 + USB 时钟源 + 中断配置
// 说明：系统时钟（XTAL/PLL/分频）统一在 mysys.c 的 bsp_clk_init() 里配置，
//       本文件不再重复配置系统时钟，只负责 USB 专属部分

#include "usb_bsp.h"
#include "hc32_ll.h"
#include "usb_dev_int.h"

// =============== USB FS 引脚 ===============
#define USB_DP_PORT                     (GPIO_PORT_A)
#define USB_DP_PIN                      (GPIO_PIN_12)
#define USB_DM_PORT                     (GPIO_PORT_A)
#define USB_DM_PIN                      (GPIO_PIN_11)

extern usb_core_instance usb_dev;

// =============== USB 中断回调 ===============
static void USB_IrqCallback(void)
{
    usb_isr_handler(&usb_dev);
}

// =============== USB 时钟源 ===============
// 必须在 mysys.c 里把 PLLQ 配成 48MHz（本工程 PLLQ = VCO 480MHz / 10）
static void UsbClockInit(void)
{
    CLK_SetUSBClockSrc(CLK_USBCLK_PLLQ);
}

// =============== USB BSP 初始化 ===============
void usb_bsp_init(usb_core_instance *pdev, stc_usb_port_identify *pstcPortIdentify)
{
    (void)pdev;
    (void)pstcPortIdentify;
    stc_gpio_init_t stcGpioCfg;

    // 解开外设写保护
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU);

    // USB 时钟源选择 PLLQ
    UsbClockInit();

    // DP/DM 配置为模拟功能
    (void)GPIO_StructInit(&stcGpioCfg);
    stcGpioCfg.u16PinAttr = PIN_ATTR_ANALOG;
    (void)GPIO_Init(USB_DM_PORT, USB_DM_PIN, &stcGpioCfg);
    (void)GPIO_Init(USB_DP_PORT, USB_DP_PIN, &stcGpioCfg);

    // 使能 USBFS 外设时钟
    FCG_Fcg1PeriphClockCmd(FCG1_PERIPH_USBFS, ENABLE);
}

// =============== USB NVIC 配置 ===============
void usb_bsp_nvicconfig(usb_core_instance *pdev)
{
    (void)pdev;
    stc_irq_signin_config_t stcIrqRegiConf;

    // 把 USBFS 全局中断挂到 INT014
    stcIrqRegiConf.enIRQn      = INT014_IRQn;
    stcIrqRegiConf.enIntSrc    = INT_SRC_USBFS_GLB;
    stcIrqRegiConf.pfnCallback = &USB_IrqCallback;
    (void)INTC_IrqSignIn(&stcIrqRegiConf);

    NVIC_ClearPendingIRQ(stcIrqRegiConf.enIRQn);
    NVIC_SetPriority(stcIrqRegiConf.enIRQn, DDL_IRQ_PRIO_15);
    NVIC_EnableIRQ(stcIrqRegiConf.enIRQn);
}

// =============== 微秒延时 ===============
void usb_udelay(const uint32_t usec)
{
    __IO uint32_t i;
    uint32_t j;
    j = (HCLK_VALUE + 1000000UL - 1UL) / 1000000UL * usec;
    for (i = 0UL; i < j; i++) {
    }
}

// =============== 毫秒延时 ===============
void usb_mdelay(const uint32_t msec)
{
    usb_udelay(msec * 1000UL);
}
