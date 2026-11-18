#include "mysys.h"
#include "mygpio.h"
#include "i2c.h"
#include "rtthread.h"

static void  SysTick_Configuration(void);

void hardware_init(void) {
    LL_PERIPH_WE(LL_PERIPH_ALL);
    BSP_CLK_Init();
    mygpio_init();
    i2c1_init();
    i2c3_init();
    SystemCoreClockUpdate();
    SysTick_Configuration();
}

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

char rt_hw_console_getchar() {
    return 0;
}

void rt_hw_console_output(const char *str) {
    RT_UNUSED(str);
}