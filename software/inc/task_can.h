#ifndef TASK_CAN_H
#define TASK_CAN_H

#include "rtthread.h"

#define CAN_STACK_SIZE          1024
#define CAN_PRIORITY            30
#define CAN_THREAD_TIMESLICE    10


extern rt_thread_t can_rx_handle;

extern rt_sem_t can_rx_sem; // 接收信号量
extern rt_sem_t can_tx_sem; // 发送信号量

void task_can_init(void);

#endif