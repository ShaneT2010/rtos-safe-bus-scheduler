#include "rtos_tasks.h"
#include "dht22_bsp.h"
#include <stdio.h>
#include <stdlib.h>

// 模擬高優先權任務：高頻率控制硬體（如馬達、PID 閉迴圈），不容許任何延遲
void vTaskMotorControl(void *pvParameters) {
    // 此任務每隔幾百個微秒就要進來處理一次
    // 當它進來時，會強行中斷正在傳輸的單線總線
    dht22_delay_us(120); // 模擬執行高強度數學運算與暫存器刷新
}

// 模擬低優先權任務：讀取環境溫濕度數據
void vTaskSensorLogging(void *pvParameters) {
    uint8_t rx_data[5] = {0};
    uint8_t checksum = 0;
    
    // 開始傳送 40-bit 數據
    for (int i = 0; i < 40; i++) {
        // 【核心搶佔模擬點】
        // 假設在讀取到第 16 個 bit (資料傳輸中途) 時，核心 Tick 剛好響起
        // 高優先權的 vTaskMotorControl 搶占 CPU 資源
        if (i == 16) {
            simulate_context_switch_injection(i);
        }
        
        rx_data[i/8] <<= 1;
        uint8_t bit_val = dht22_read_bit();
        
        if (bit_val == 1) {
            rx_data[i/8] |= 0x01;
        } else if (bit_val == 0xFF) {
            // 偵測到時序崩壞引起的 Timeout
            break;
        }
    }
}

// 模擬 FreeRTOS 搶占時造成的微秒級的時序撕裂
void simulate_context_switch_injection(uint32_t bit_count) {
    printf("[KERNEL] vTaskSensorLogging is running (Bit %d)... \n", bit_count);
    printf("[KERNEL] !!! PREEMPTIVE TRIGGER !!! Higher priority task 'vTaskMotorControl' pre-empts CPU.\n");
    
    // 呼叫高優先權任務，強行卡死 CPU 120微秒
    vTaskMotorControl(NULL);
    
    // 模擬 Context Switch 造成的額外硬體上下文切換延遲 (儲存暫存器、切換堆疊)
    dht22_delay_us(35); 
    
    printf("[KERNEL] vTaskMotorControl suspended. Returning execution back to vTaskSensorLogging.\n");
    printf("[KERNEL] Total timing degradation injected: 155us. Bus state corrupted.\n");
}

void rtos_kernel_init(void) {
    printf("[KERNEL] Bootstrapping FreeRTOS Preemptive Core...\n");
    printf("[KERNEL] Task 'vTaskMotorControl' spawned at Priority %d\n", MOTOR_CONTROL_TASK_PRIORITY);
    printf("[KERNEL] Task 'vTaskSensorLogging' spawned at Priority %d\n", SENSOR_LOGGING_TASK_PRIORITY);
    printf("[KERNEL] Kernel Scheduler started. Running stress topology.\n");
    
    // 執行一輪多工壓測模擬
    vTaskSensorLogging(NULL);
}
