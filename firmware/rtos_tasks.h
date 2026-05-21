#ifndef RTOS_TASKS_H
#define RTOS_TASKS_H

#include <stdint.h>

// 模擬 FreeRTOS 的作業系統 Scheduling Constants
#define configMAX_PRIORITIES           (5)
#define tskIDLE_PRIORITY               (0)

// 定義兩個互相衝突的任務優先順序
// 高優先權的馬達控制會 Preempt 低優先權的感測器傳輸
#define MOTOR_CONTROL_TASK_PRIORITY    (4) // Higher Priority
#define SENSOR_LOGGING_TASK_PRIORITY   (1) // Lower Priority

// 模擬 FreeRTOS 核心控制塊 (Simulated TCB & Kernel States)
typedef struct {
    uint32_t task_id;
    uint32_t priority;
    const char* task_name;
} TaskControlBlock_t;

// 函式宣告
void rtos_kernel_init(void);
void vTaskMotorControl(void *pvParameters);
void vTaskSensorLogging(void *pvParameters);
void simulate_context_switch_injection(uint32_t bit_count);

#endif // RTOS_TASKS_H
