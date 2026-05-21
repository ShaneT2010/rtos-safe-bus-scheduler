#ifndef WINDOW_SCHEDULER_H
#define WINDOW_SCHEDULER_H

#include <stdint.h>

// 定義核心硬體核心暫存器位址 (Cortex-M Core Peripherals)
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

// 函式宣告
uint8_t dht22_read_bit_with_defense(void);

#endif // WINDOW_SCHEDULER_H
