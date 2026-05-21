// firmware/one_wire_bus.h
#ifndef ONE_WIRE_BUS_H
#define ONE_WIRE_BUS_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"

// ============================================================================
// [原本保留] 總線常數與 ROM 指令定義
// ============================================================================
#define MAX_BUS_NODES         4
#define ROM_BIT_LENGTH        64

#define CMD_SEARCH_ROM        0xF0
#define CMD_READ_ROM          0x33
#define CMD_MATCH_ROM         0x55
#define CMD_SKIP_ROM          0xCC

// ============================================================================
// [重構優化] 拆解與擴充 64-bit ROM 結構 (使其相容原本的 8-byte 陣列與二元樹操作)
// ============================================================================
typedef struct {
    uint8_t family_code;    // 8-bit 家族代碼 (1st Byte)
    uint8_t serial_num[6];  // 48-bit 獨一無二序列號 (2nd ~ 7th Byte)
    uint8_t crc8;           // 8-bit 校驗碼 (8th Byte)
} one_wire_rom_t;

typedef struct {
    union {
        uint8_t rom_id[8];         // [原本保留] 保持與第一版相容的 8-bytes 陣列規格
        one_wire_rom_t rom_fields; // [Phase 6.1 新增] 方便二元樹演算法存取的欄位
    };
    uint8_t   is_virtual;       // [原本保留] 1 = Emulated in Track A, 0 = Physical Node
    float     last_temperature; // [原本保留] 環境溫濕度快取
    float     last_humidity;    // [原本保留]
} one_wire_node_t;

// ============================================================================
// [完美融合] 1-Wire 總線核心控制結構體
// ============================================================================
typedef struct {
    // --- 原本保留的快取、數量與資源鎖 ---
    one_wire_node_t   nodes[MAX_BUS_NODES];
    uint8_t           discovered_count;
    SemaphoreHandle_t bus_mutex;

    // --- Phase 6.1 補強的核心二元樹搜尋狀態機 ---
    uint8_t           last_discrepancy;        // 上一次發生碰撞(0/1分歧)的位元位置
    uint8_t           last_family_discrepancy; // 家族代碼區段的最後分歧點
    uint8_t           last_device_flag;        // 1 = 已搜尋完最後一個元件，終止搜尋迴圈
    one_wire_rom_t    current_rom;             // 當前疊代正在分离/掃描中的 ROM ID
} one_wire_bus_t;

// ============================================================================
// [完全閉環] Core 1-Wire Driver API 宣告
// ============================================================================
void    OneWire_InitBus(one_wire_bus_t *bus); // 對齊第一版命名
uint8_t OneWire_SearchROM(one_wire_bus_t *bus);
void    OneWire_InjectVirtualNode(one_wire_bus_t *bus, uint8_t *mock_rom);

#endif /* ONE_WIRE_BUS_H */
