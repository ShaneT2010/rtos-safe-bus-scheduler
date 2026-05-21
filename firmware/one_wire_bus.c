#include "one_wire_bus.h"
#include "rtos_tasks.h"  // 引入 taskENTER_CRITICAL / taskEXIT_CRITICAL
#include <string.h>
#include <stdio.h>

// ============================================================================
// [DPI-C 橋梁宣告] 讓 C 韌體在模擬環境中能推動 SystemVerilog 的時間軸與硬體介面
// ============================================================================
extern void ow_bus_reset(void);
extern uint8_t ow_read_bit(void);
extern void ow_write_bit(uint8_t bit_val);
extern void sv_delay_us(int us);

// 覆蓋實體板子的延遲函數：確保 Pre-Silicon 模擬時間前進
static void delay_us(int us) {
    sv_delay_us(us);
}

// ============================================================================
// 初始化總線結構體與互斥鎖 (對齊新舊結構體欄位)
// ============================================================================
void OneWire_InitBus(one_wire_bus_t *bus) {
    bus->discovered_count = 0;
    bus->last_discrepancy = 0;         // 整合至 Phase 6.1 標準狀態機名稱
    bus->last_family_discrepancy = 0;
    bus->last_device_flag = 0;
    bus->bus_mutex = xSemaphoreCreateMutex();
    
    memset(bus->nodes, 0, sizeof(bus->nodes));
    memset(bus->current_rom.rom_id, 0, 8);
}

// ============================================================================
// Track A 虛擬節點注入機制
// ============================================================================
void OneWire_InjectVirtualNode(one_wire_bus_t *bus, uint8_t *mock_rom) {
    if (bus->discovered_count < MAX_BUS_NODES) {
        // 利用 union 特性，不論上層寫入 rom_id 還是底下讀取欄位，記憶體完全同步
        memcpy(bus->nodes[bus->discovered_count].rom_id, mock_rom, 8);
        bus->nodes[bus->discovered_count].is_virtual = 1;
        bus->discovered_count++;
        printf("[1-WIRE HAL] Virtual Node successfully injected into slot %d.\n", bus->discovered_count - 1);
    }
}

// ============================================================================
// [完美融合版] 全球標準 1-Wire 二元樹 ROM 搜尋演算法
// ============================================================================
uint8_t OneWire_SearchROM(one_wire_bus_t *bus) {
    uint8_t id_bit_number = 1;
    uint8_t last_zero = 0;
    uint8_t rom_byte_number = 0;
    uint8_t rom_byte_mask = 1;
    uint8_t search_result = 0;
    
    uint8_t id_bit, cmp_id_bit;
    uint8_t search_direction = 0;

    // 1. 安全防禦：若上一輪已確認為最後一個元件，直接復歸結束
    if (bus->last_device_flag) {
        return 0;
    }

    // 2. 獲取 RTOS 互斥鎖，防止多任務同時調用位元碰撞
    if (xSemaphoreTake(bus->bus_mutex, portMAX_DELAY) != pdTRUE) {
        return 0;
    }

    // ========================================================================
    // 【核心防禦點】進入 FreeRTOS 臨界區，鎖死硬體中斷，阻止時序被中斷撕裂
    // ========================================================================
    taskENTER_CRITICAL();

    // 3. 發送 1-Wire 標準重置與存在脈衝 (Reset & Presence Pulse)
    // 核心修正：將原先錯誤的 dht22_bus_reset 改為新一代導出的 ow_bus_reset
    ow_bus_reset(); 
    delay_us(100); // 重置後的穩定沉澱時間

    // 4. 開始 64 位元二元樹分支遍歷
    while (id_bit_number <= ROM_BIT_LENGTH) {
        
        // 核心硬體讀取：實時對 GPIO 引腳進行微秒級採樣（原位元與反轉位元）
        // 核心修正：對齊正確的 ow_read_bit 接口，不再調用 DHT22 舊引腳
        id_bit     = ow_read_bit(); 
        cmp_id_bit = ow_read_bit();

        if ((id_bit == 1) && (cmp_id_bit == 1)) {
            // 情況 1：無任何硬體或虛擬節點響應總線，通訊崩潰
            break;
        } 
        else {
            if (id_bit != cmp_id_bit) {
                // 情況 2 & 3：無碰撞，所有感測器在該位元的方向一致（均為 0 或均為 1）
                search_direction = id_bit;
            } 
            else {
                // 情況 4：【偵測到多節點碰撞 (00)！】進入二元樹決策分支
                if (id_bit_number < bus->last_discrepancy) {
                    // 若還沒走到上一次發生分歧的極限起點，沿用上一輪走過的路徑
                    search_direction = ((bus->current_rom.rom_id[rom_byte_number] & rom_byte_mask) > 0);
                } 
                else if (id_bit_number == bus->last_discrepancy) {
                    // 若剛好踩在上一輪的最後分歧點，這一輪強迫改走「1」的分支
                    search_direction = 1;
                } 
                else {
                    // 遇到全新未知的碰撞點，二元樹演算法優先探測「0」的分支
                    search_direction = 0;
                }

                // 紀錄最後一次選擇走 0 的分歧點絕對位元位置
                if (search_direction == 0) {
                    last_zero = id_bit_number;
                    
                    // 額外紀錄：若碰撞點發生在前 8 位元，保存為家族代碼分歧點
                    if (id_bit_number <= 8) {
                        bus->last_family_discrepancy = id_bit_number;
                    }
                }
            }

            // 將主控端抉擇出來的方向，透過位元遮罩（Bit-Mask）寫入當前緩衝區
            if (search_direction == 1) {
                bus->current_rom.rom_id[rom_byte_number] |= rom_byte_mask;
            } else {
                bus->current_rom.rom_id[rom_byte_number] &= ~rom_byte_mask;
            }

            // 實體回寫：通知總線上的所有感測器，不符合此方向的感測器自動進入休眠（淘汰）
            // 核心修正：對齊正確的 ow_write_bit 接口
            ow_write_bit(search_direction);

            // 步進前移至 64-bit 空間中的下一個位元
            id_bit_number++;
            rom_byte_mask <<= 1;
            if (rom_byte_mask == 0) {
                rom_byte_number++;
                rom_byte_mask = 1; // 跨越 Byte，遮罩歸位
            }
        }
    } // 64-bit 遍歷結束

    // 5. 狀態機參數更新，為下一輪演算法遞迴迭代做足跡儲存
    bus->last_discrepancy = last_zero;
    
    if (bus->last_discrepancy == 0) {
        bus->last_device_flag = 1; // 找不到任何 0 分支，代表整棵二元樹探測完畢！
    }

    // 6. 如果順利走完 64 位元，將當前分離出來的 ROM 存入暫存快取陣列中
    if (id_bit_number == 65) {
        if (bus->discovered_count < MAX_BUS_NODES) {
            // 將搜尋到的實體硬體節點存檔
            memcpy(bus->nodes[bus->discovered_count].rom_id, bus->current_rom.rom_id, 8);
            bus->nodes[bus->discovered_count].is_virtual = 0; // 標記為實體掃描所獲
            bus->discovered_count++;
        }
        search_result = 1; // 成功分離出一個節點
    }

    // ========================================================================
    // 退出臨界區，還原 FreeRTOS 排班器
    // ========================================================================
    taskEXIT_CRITICAL();
    
    // 釋放軟體互斥鎖
    xSemaphoreGive(bus->bus_mutex);
    
    return search_result;
}

// ============================================================================
// [全新修復引入] DPI-C 測試進入點主任務：負責驅動整個韌體層的搜尋循環
// ============================================================================
void c_run_one_wire_test(void) {
    one_wire_bus_t system_bus;
    uint8_t res;
    int loop_guard = 0;

    printf("[C FIRMWARE] One-Wire SearchROM Engine Invoked via DPI-C Context.\n");
    
    // 初始化主韌體驅動結構體
    OneWire_InitBus(&system_bus);

    // 迴圈遍歷二元樹，直到 last_device_flag 被立起 (最多防禦 10 次避免掛機)
    while (system_bus.last_device_flag == 0 && loop_guard < 10) {
        loop_guard++;
        printf("[C FIRMWARE] Initiating SearchROM Pass #%d...\n", loop_guard);
        
        res = OneWire_SearchROM(&system_bus);
        
        if (res) {
            uint32_t idx = system_bus.discovered_count - 1;
            printf("[C FIRMWARE] >>> Success! Found Node %d: ROM = %02X%02X%02X%02X%02X%02X%02X%02X\n",
                   idx,
                   system_bus.nodes[idx].rom_id[7], system_bus.nodes[idx].rom_id[6],
                   system_bus.nodes[idx].rom_id[5], system_bus.nodes[idx].rom_id[4],
                   system_bus.nodes[idx].rom_id[3], system_bus.nodes[idx].rom_id[2],
                   system_bus.nodes[idx].rom_id[1], system_bus.nodes[idx].rom_id[0]);
        } else {
            printf("[C FIRMWARE] >>> Pass #%d did not discover any active nodes.\n", loop_guard);
        }
    }

    printf("[C FIRMWARE] SearchROM Process Finished. Total discovered nodes: %d\n", system_bus.discovered_count);
}
