#ifndef PWR_CONTROLLER_H
#define PWR_CONTROLLER_H

#include <stdint.h>

// 映射 Cortex-M 系統控制暫存器 (System Control Register, SCR)
// 控制核心睡眠深度
#define CPU_SCR_REG            (*(volatile uint32_t *)(0xE000ED10UL))
#define SCR_SLEEPDEEP_BIT      (1UL << 2) // 啟用深度睡眠 (Stop Mode)

// 模擬 STM32 電源控制暫存器 (Power Control Register, PWR_CR)
#define MCU_PWR_BASE           (0x40007000UL)
#define MCU_PWR_CR             (*(volatile uint32_t *)(MCU_PWR_BASE + 0x00UL))
#define PWR_CR_LPDS_BIT        (1UL << 0) // 低功耗調壓器配置 (Low-power Deep Sleep)

// 函式宣告
void pwr_infrastructure_init(void);
void vApplicationSleepProcessing(uint32_t xExpectedIdleTime);

#endif // PWR_CONTROLLER_H
