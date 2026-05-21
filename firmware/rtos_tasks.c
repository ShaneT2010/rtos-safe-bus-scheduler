// firmware/rtos_tasks.c
#include "rtos_tasks.h"
#include "dht22_bsp.h"
#include "one_wire_bus.h"      // [Phase 6.1 新增] 引入 1-Wire 總線驅動
#include "window_scheduler.h"  // [Phase 6.1 新增] 引入視窗排班器控制 API
#include <stdio.h>
#include <stdlib.h>

// 宣告全域 1-Wire 匯流排控管結構體
one_wire_bus_t g_1wire_bus;

// ============================================================================
// [原本保留] 高優先權任務：高頻率控制硬體（如馬達、PID 閉迴圈）
// ============================================================================
void vTaskMotorControl(void *pvParameters) {
    // 此任務每隔幾百個微秒就要進來處理一次
    // 當它進來時，會強行中斷正在傳輸的單線總線
    dht22_delay_us(120); // 模擬執行高強度數學運算與暫存器刷新
}

// ============================================================================
// [重構升級] 低優先權任務：讀取環境溫濕度數據 + 1-Wire 拓撲多節點搜尋
// ============================================================================
void vTaskSensorLogging(void *pvParameters) {
    uint8_t rx_data[5] = {0};
    uint8_t find_next_node = 1;
    
    // --- 1-Wire 總線硬體抽象層初始化 ---
    OneWire_Init(&g_1wire_bus);

    // ========================================================================
    // 測試流程 A：[原本保留] 模擬原本 DHT22 的 40-bit 讀取與中斷強行搶佔壓測
    // ========================================================================
    printf("[TASK] Starting Test Loop A: Standard DHT22 Timing Preemption Test.\n");
    
    // 開始傳送 40-bit 數據
    for (int i = 0; i < 40; i++) {
        // 假設在讀取到第 16 個 bit 時，核心 Tick 響起，高優先權任務搶占
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

    // ========================================================================
    // 測試流程 B：[Phase 6.1 新增] 1-Wire 二元樹搜尋演算法與atom臨界區防禦
    // ========================================================================
    printf("\n[TASK] Starting Test Loop B: 1-Wire Multi-Drop Search ROM with Critical Section.\n");
    
    // 1. 通知 window_scheduler：開始掃描二元樹了，全面關閉原本的 6.2ms 時間避讓機制
    Window_Set1WireSearchState(1);
    
    g_1wire_bus.last_discrepancy = 0;
    g_1wire_bus.last_device_flag = 0;
    find_next_node = 1;
    
    while (find_next_node) {
        printf("[1-WIRE] Executing SearchROM iteration...\n");
        
        // 2. 呼叫核心二元樹演算法。
        // 注意：在 one_wire_bus.c 的 OneWire_SearchROM 內部，會主動呼叫
        // taskENTER_CRITICAL()。此時即使發生中斷、或呼叫 simulate_context_switch_injection，
        // CPU 都會強制鎖定，波形絕對不會像測試流程 A 一樣被撕裂！
        find_next_node = OneWire_SearchROM(&g_1wire_bus);
        
        // 3. 每搜尋完一個節點，短暫讓出 CPU（10ms），
        // 讓累積的高優先權任務（vTaskMotorControl）有機會被執行，避免馬達控制發生飢餓
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // 4. 整個拓撲掃描結束，把防禦權還給 window_scheduler，恢復原本單次讀取的預判邏輯
    Window_Set1WireSearchState(0);
    printf("[1-WIRE] Topology discovery completed successfully.\n");
}

// ============================================================================
// [原本保留] 模擬 FreeRTOS 搶占時造成的微秒級的時序撕裂
// ============================================================================
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

// ============================================================================
// [原本保留] 核心初始化與測試啟動
// ============================================================================
void rtos_kernel_init(void) {
    printf("[KERNEL] Bootstrapping FreeRTOS Preemptive Core...\n");
    printf("[KERNEL] Task 'vTaskMotorControl' spawned at Priority %d\n", MOTOR_CONTROL_TASK_PRIORITY);
    printf("[KERNEL] Task 'vTaskSensorLogging' spawned at Priority %d\n", SENSOR_LOGGING_TASK_PRIORITY);
    printf("[KERNEL] Kernel Scheduler started. Running stress topology.\n");
    
    // 執行一輪多工壓測模擬
    vTaskSensorLogging(NULL);
}
