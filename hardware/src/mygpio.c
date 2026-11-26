#include "mygpio.h"

void os_led_init(void){
    stc_gpio_init_t stcGpioInit;
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinState = PIN_STAT_SET;
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    (void)GPIO_Init(OS_LED_PORT, OS_LED_PIN, &stcGpioInit);
}
