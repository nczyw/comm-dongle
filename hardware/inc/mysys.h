#ifndef MYSYS_H
#define MYSYS_H

#include "hc32_ll.h"

#define BSP_XTAL_PORT                   (GPIO_PORT_F)
#define BSP_XTAL_PIN                    (GPIO_PIN_00 | GPIO_PIN_01)


void bsp_clk_init(void);

#endif
