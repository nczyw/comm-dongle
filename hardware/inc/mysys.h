#ifndef MY_SYS_H
#define MY_SYS_H

#include "hc32_ll.h"

#define BSP_XTAL_PORT                   (GPIO_PORT_H)
#define BSP_XTAL_PIN                    (GPIO_PIN_00 | GPIO_PIN_01)


void BSP_CLK_Init(void);

#endif


