#include "rtthread.h"
#include "mysys.h"
#include "mygpio.h"

static void  SysTick_Configuration(void);

void hardware_init(void) {
    LL_PERIPH_WE(LL_PERIPH_ALL);
    bsp_clk_init();     // 初始化系统时钟
    SystemCoreClockUpdate();
    SysTick_Configuration();
}

/*******************************************************************************
 * Function Name  : SysTick_Configuration
 * Description    : Configures the SysTick for OS tick.
 * Input          : None
 * Output         : None
 * Return         : None
 *******************************************************************************/
static void  SysTick_Configuration(void)
{
    stc_clock_freq_t stcClkFreq;
    rt_uint32_t cnts;

    CLK_GetClockFreq(&stcClkFreq);

    cnts = (rt_uint32_t)stcClkFreq.u32HclkFreq / RT_TICK_PER_SECOND;

    SysTick_Config(cnts);
}


void rt_hw_us_delay(rt_uint32_t us){
   uint32_t start, now, delta, reload, us_tick;
   start = SysTick->VAL;
   reload = SysTick->LOAD;
   us_tick = SystemCoreClock / 1000000UL;  // 计算 1us 多少个时钟周期

   rt_kprintf("SysTick->LOAD: %lu, us_tick: %lu\n", reload, us_tick);

   do
   {
       now = SysTick->VAL;
       delta = start > now ? start - now : reload + start - now;
   }
   while (delta < us_tick * us);
}