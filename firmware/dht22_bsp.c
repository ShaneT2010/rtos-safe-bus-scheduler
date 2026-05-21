#include "dht22_bsp.h"

// 1. 初始化 GPIO 為 Open-Drain，即單線總線 1-Wire 的標準配置
void dht22_bsp_init(void) {
    // 將 GPIOA Pin 5 設定為 Output Mode (01)
    GPIOA_MODER &= ~(3UL << (DHT22_PIN * 2));
    GPIOA_MODER |=  (1UL << (DHT22_PIN * 2));
    
    // 將輸出型態設定為 Open-Drain，允許外部上拉電阻將電位拉高
    // 這樣當輸出 HIGH 時，線路其實是放空（高阻抗），傳感器才能把線路拉低
    (*(volatile uint32_t *)(GPIOA_BASE + 0x04UL)) |= (1UL << DHT22_PIN);
    
    DHT22_BUS_HIGH(); // 初始狀態放開總線 (High Level)
}

// 2. 微秒級阻塞精確延時 (利用 DWT 外設或簡單指令週期估算)
void dht22_delay_us(uint32_t us) {
    // 這裡預留核心計時器 (Data Watchpoint and Trace, DWT) 暫存器操作
    // 作為 Pre-Silicon 到 Post-Silicon 轉換的精確度保證
    volatile uint32_t cycles = us * 84; // 先假設 MCU 運行在 84MHz
    while(cycles--) {
        __asm("nop"); // 插入組合語言空指令，防止編譯器優化掉此迴圈
    }
}

// 3. 讀取單個位元：這段代碼的時序須完美對齊 Phase 1 的 SVA 規則
uint8_t dht22_read_bit(void) {
    uint32_t retry = 0;
    
    // 等待 Low 電位結束 (對應 Phase 1 的 T_BIT_LOW = 50us)
    while(!DHT22_BUS_READ()) {
        if(retry++ > 10000) return 0xFF; // 防卡死安全機制 (Timeout)
    }
    
    // 進入 High 電位後，延時 40us 後進行採樣
    // 呼應 Phase 1：26us 代表邏輯 0，70us 代表邏輯 1
    // 如果 40us 後讀到的是 Low，代表它只亮了 26us 就倒下了 (Logic 0)
    // 如果 40us 後讀到的是 High，代表它會持續亮到 70us (Logic 1)
    dht22_delay_us(40);
    
    if(DHT22_BUS_READ()) {
        // 既然是 High，繼續等待傳感器釋放總線，直到變回 Low，為下一個 bit 做準備
        retry = 0;
        while(DHT22_BUS_READ()) {
            if(retry++ > 10000) return 0xFF;
        }
        return 1; // 判定為邏輯 1
    } else {
        return 0; // 判定為邏輯 0
    }
}
