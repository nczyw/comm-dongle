#ifndef MY_GPIO_H
#define MY_GPIO_H

#include "hc32_ll.h"

#define OS_LED_PORT        (GPIO_PORT_F)
#define OS_LED_PIN         (GPIO_PIN_02)
#define OS_LED()           (GPIO_TogglePins(OS_LED_PORT, OS_LED_PIN))
/* 熄灭：恢复初始化时的高电平（若硬件是输出高点亮，改成 GPIO_ResetPins） */
#define OS_LED_OFF()       (GPIO_SetPins(OS_LED_PORT, OS_LED_PIN))

void os_led_init(void);

#endif