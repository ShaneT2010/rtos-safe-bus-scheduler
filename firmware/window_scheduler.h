// firmware/window_scheduler.h
#ifndef WINDOW_SCHEDULER_H
#define WINDOW_SCHEDULER_H

#include <stdint.h>

// ============================================================================
// [原本保留] 核心硬體核心暫存器位址與時序常數 (Cortex-M Core Peripherals)
// ============================================================================
#define CPU_CLOCK_HZ          (84000000UL)  // 84 MHz System Clock
#define RTOS_TICK_RATE_HZ     (1000UL)      // FreeRTOS 1ms Tick
#define SYSTICK_LOAD_VALUE    ((CPU_CLOCK_HZ / RTOS_TICK_RATE_HZ) - 1) // 83,999

// 映射 SysTick 當前計數值暫存器 (Current Value Register)
// 在 Cortex-M 中，這個暫存器會從 83999 遞減到 0，當到 0 的瞬間，就會引發 FreeRTOS 的 Context Switch 中斷
#define SYSTICK_VAL_REG       (*(volatile uint32_t *)(0xE000E018UL))

// 臨界通訊窗口常數
// 傳送一個 Logic '1' 需要：50us (Low) + 70us (High) = 120us
// 加上 30us 的 Safety Margin，安全窗口定為 150us
#define SYSTEM_SAFE_WINDOW_US  (150UL)
#define SYSTEM_SAFE_CYCLES     (SYSTEM_SAFE_WINDOW_US * (CPU_CLOCK_HZ / 1000000UL)) // 12,600 週期


// ============================================================================
// [Phase 6.1 新增] 1-Wire 多節點二元樹搜尋防禦機制與狀態返回值
// ============================================================================
#define WINDOW_SAFE   1
#define WINDOW_UNSAFE 0

// 原本 Phase 4 遺留的 DHT22 單次讀取防禦預判函式
uint8_t Window_CheckSafeToRead(void);

// [Phase 6.1 新增] 動態通知視窗排班器：1-Wire 二元樹搜尋是否正在動工
// 當 active = 1 時，將 Bypass 原本的 SysTick 視窗檢查，改由臨界區原子鎖防守
void Window_Set1WireSearchState(uint8_t active);


// ============================================================================
// [原本保留] 既有防禦讀取函式宣告
// ============================================================================
uint8_t dht22_read_bit_with_defense(void);

#endif // WINDOW_SCHEDULER_H
