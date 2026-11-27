#ifndef MY_GPIO_H
#define MY_GPIO_H

#include "hc32_ll.h"

#define OS_LED_PORT        (GPIO_PORT_F)
#define OS_LED_PIN         (GPIO_PIN_02)
#define OS_LED_ON()        (GPIO_ResetPins(OS_LED_PORT, OS_LED_PIN))
#define OS_LED_OFF()       (GPIO_SetPins(OS_LED_PORT, OS_LED_PIN))

void os_led_init(void);

#endif