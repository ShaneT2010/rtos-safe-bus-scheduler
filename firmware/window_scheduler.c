// firmware/window_scheduler.c
#include "window_scheduler.h"
#include "dht22_bsp.h"
#include "rtos_tasks.h" // 引入 FreeRTOS / 任務控制相關標頭檔
#include <stdio.h>

// [Phase 6.1 新增] 標記 1-Wire 匯流排是否正處於高強度的二元樹掃描狀態
static volatile uint8_t g_1wire_search_active = 0;

// [Phase 6.1 新增] 動態切換搜尋狀態
void Window_Set1WireSearchState(uint8_t active) {
    g_1wire_search_active = active;
}

// ============================================================================
// [原本保留] 動態主動位元讀取函式 (兼具 Phase 6.1 臨界區 Bypass 機制)
// ============================================================================
uint8_t dht22_read_bit_with_defense(void) {
    uint32_t current_systick_cycles;
    uint32_t time_left_until_next_tick_us;

    // 【Phase 6.1 新增核心防禦分流】
    // 如果當前正在進行 1-Wire 多節點二元樹搜尋，底層 one_wire_bus.c 內部
    // 已經使用了 taskENTER_CRITICAL() 將中斷與 Context Switch 完全鎖死。
    // 此時匯流排擁有绝对原子保護，SysTick 的主動避讓（會引入 delay）反而會破壞時序，
    // 故直接 Bypass 避讓演算法，放行執行精確的硬體位元讀取！
    if (g_1wire_search_active) {
        return dht22_read_bit();
    }

    // --- 以下為原本保留且完好無損的 SysTick 微秒級主動避讓演算法 ---
    
    // 讀取當前 SysTick 遞減計數器的數值
    current_systick_cycles = SYSTICK_VAL_REG;

    // 因為 SysTick 是倒數計時器，當前數值代表距離下一次 1ms 中斷還剩下多少個 clock 週期，轉換為微秒
    time_left_until_next_tick_us = current_systick_cycles / (CPU_CLOCK_HZ / 1000000UL);

    // 檢查剩餘時間是否小於安全傳輸窗口 (150us)
    if (current_systick_cycles < SYSTEM_SAFE_CYCLES) {
        printf("[DEFENSE ENGINE] Collision Imminent! Only %dus left before RTOS preemption.\n", 
               time_left_until_next_tick_us);
        printf("[DEFENSE ENGINE] Actively yielding CPU. Forcing deferral to bypass danger zone.\n");

        // 讓 FreeRTOS 的中斷先在背景安全執行完畢（高優先權的馬達控制優先跑完）
        // 設計理由：自願地主動延遲，延遲時間直接跨過此次 Tick 臨界點（此處設置延遲 200us）
        dht22_delay_us(200);

        printf("[DEFENSE ENGINE] Resuming transaction in a freshly synchronized safe window.\n");
    }

    // 避開危險期後，或本來就處於安全期即可執行精確的硬體位元讀取
    return dht22_read_bit();
}

// ============================================================================
// [Phase 6.1 新增] 一般性通用安全視窗檢查介面 (供驅動層其他擴充功能調用)
// ============================================================================
uint8_t Window_CheckSafeToRead(void) {
    if (g_1wire_search_active) {
        return WINDOW_SAFE; 
    }
    
    // 檢查當前 SysTick 是否落在危險窗口內
    if (SYSTICK_VAL_REG < SYSTEM_SAFE_CYCLES) {
        return WINDOW_UNSAFE;
    }
    
    return WINDOW_SAFE;
}
