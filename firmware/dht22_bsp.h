#ifndef DHT22_BSP_H
#define DHT22_BSP_H

#include <stdint.h>

// 使用 STM32F4 系列，腳位定義為 GPIOA, Pin 5 
// 直接映射暫存器記憶體位址 (Memory-mapped Registers)
#define GPIOA_BASE         (0x40020000UL)
#define GPIOA_MODER        (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_ODR          (*(volatile uint32_t *)(GPIOA_BASE + 0x14UL))
#define GPIOA_IDR          (*(volatile uint32_t *)(GPIOA_BASE + 0x10UL))

#define DHT22_PIN          (5)

// 巨集指令：利用暫存器實現微秒級的快速位元操作 (Bit Manipulation)
#define DHT22_BUS_LOW()    (GPIOA_ODR &= ~(1UL << DHT22_PIN))
#define DHT22_BUS_HIGH()   (GPIOA_ODR |= (1UL << DHT22_PIN))
#define DHT22_BUS_READ()   ((GPIOA_IDR >> DHT22_PIN) & 0x01UL)

// 函式宣告
void dht22_bsp_init(void);
void dht22_delay_us(uint32_t us);
uint8_t dht22_read_bit(void);

#endif // DHT22_BSP_H
