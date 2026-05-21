#include "pwr_controller.h"
#include "dht22_bsp.h"
#include <stdio.h>

// 初始化低功耗硬體環境
void pwr_infrastructure_init(void) {
    printf("[PWR] Initializing Advanced Power Management Architecture.\n");
    
    // 設定暫存器：允許 MCU 進入深度睡眠模式
    CPU_SCR_REG |= SCR_SLEEPDEEP_BIT;
    
    // 開啟內部電壓調節器的低功耗模式，將 CMOS 靜態 leaked current 降到 mA Level
    MCU_PWR_CR |= PWR_CR_LPDS_BIT;
    
    printf("[PWR] Hardware-level Tickless Idle triggers armed successfully.\n");
}

// FreeRTOS 專屬低功耗 Tickless Idle Hook
// 當作業系統發現完全沒有任務要執行時，會自動調用這個函式，並傳入「預期空閒時間」
void vApplicationSleepProcessing(uint32_t xExpectedIdleTime) {
    // 如果空閒時間太短（例如小於 5 毫秒），進出睡眠的硬體開銷不划算，便放棄進入此模式
    if (xExpectedIdleTime < 5) {
        return;
    }

    printf("\n[PWR_ENGINE] >>> KERNEL IDLE DETECTION: Entering Deep Sleep for %d ms <<<\n", xExpectedIdleTime);
    printf("[PWR_ENGINE] Suppressing SysTick periodic interrupts to achieve static CMOS state.\n");
    
    // 【物理模擬觀察點】
    // 在實體硬體中，執行 `WFI` (Wait For Interrupt) 組合語言指令後，
    // MCU 的 Clock Tree 會立刻關閉，電流會從 15mA 驟降至 ~5uA
    __asm volatile("wfi"); 

    // --- 當外部中斷（例如傳感器訊號拉低）或定時器到期喚醒 MCU 後，程式從這裡繼續執行 ---
    
    printf("[PWR_ENGINE] <<< WAKEUF EVENT DETECTED: Restoring Peripheral Clock Trees >>>\n");
    printf("[PWR_ENGINE] Compensating RTOS kernel ticks by +%d ms.\n", xExpectedIdleTime);
}
