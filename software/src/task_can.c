#include "task_can.h"
#include "can.h"
#include "mygpio.h"

rt_thread_t can_rx_handle = RT_NULL;

rt_sem_t can_tx_sem = RT_NULL;
rt_sem_t can_rx_sem = RT_NULL;


static void task_can_rx_entry(void *parameter);

void task_can_init(void) {
    
    can_tx_sem = rt_sem_create("can_tx", 0, RT_IPC_FLAG_FIFO);
    if (can_tx_sem == RT_NULL) {
        OS_LED_ON();
        while (1);
    }
    can_rx_sem = rt_sem_create("can_rx", 0, RT_IPC_FLAG_FIFO);
    if (can_rx_sem == RT_NULL) {
        OS_LED_ON();
        while (1);
    }

    can_rx_handle = rt_thread_create(
        "can_rx", 
        task_can_rx_entry, 
        RT_NULL, 
        CAN_STACK_SIZE, 
        CAN_PRIORITY, 
        CAN_THREAD_TIMESLICE
    );
    if (can_rx_handle == RT_NULL) {
        OS_LED_ON();
        while (1);
    } else {
        can_init();
        rt_thread_startup(can_rx_handle);
    }
}


static void task_can_rx_entry(void *parameter) {
    RT_UNUSED(parameter);
    stc_can_rx_frame_t stc_frame;
    while (1) {
        rt_sem_take(can_rx_sem, RT_WAITING_FOREVER);
        while ( can_rx_tail != can_rx_head) {
            stc_frame = can_rx_frame[can_rx_tail];
            can_rx_tail = (uint16_t)((can_rx_tail + 1U) % CAN_RX_FRAME_NUM);
        }
        RT_UNUSED(stc_frame);
    }
}