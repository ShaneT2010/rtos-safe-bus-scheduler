#include "window_scheduler.h"
#include "dht22_bsp.h"
#include <stdio.h>

// 動態主動位元讀取函式
uint8_t dht22_read_bit_with_defense(void) {
    uint32_t current_systick_cycles;
    uint32_t time_left_until_next_tick_us;

    // 讀取當前 SysTick 遞減計數器的數值
    current_systick_cycles = SYSTICK_VAL_REG;

    // 因為 SysTick 是倒數計時器，當前數值代表距離下一次 1ms 中斷還剩下多少個clock週期，轉換為微秒
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
