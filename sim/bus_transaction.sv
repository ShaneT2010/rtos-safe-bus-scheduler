// sim/bus_transaction.sv
`timescale 1ns/1ps
import bus_types::*;

// ============================================================================
// [Phase 1-5 核心保留] DHT22 驗證交易物件 (100% 相容原本的 Test Block A)
// ============================================================================
class bus_transaction;
    // 隨機變數：模擬 FreeRTOS 核心中斷發生的時間點 (單位: us)
    rand int rtos_interrupt_offset;
    
    // 隨機變數：40-bit 的傳感器原始資料 (Raw Data)
    rand bit [39:0] sensor_raw_data;

    // 限制區塊：定義合法的隨機範圍
    constraint c_rtos_timing {
        rtos_interrupt_offset inside {[0:1000]}; 
    }

    constraint c_sensor_data {
        // 限制濕度與溫度在合理物理區間，避免無意義的隨機訊號
        sensor_raw_data[39:24] inside {[0:1000]}; // 濕度 0.0% - 100.0%
        sensor_raw_data[23:8]  inside {[0:500]};  // 溫度 0.0°C - 50.0°C
        
        // Checksum 自動計算欄位 (4個位元組相加等於低 8 位元)
        sensor_raw_data[7:0] == (sensor_raw_data[39:32] + sensor_raw_data[31:24] + 
                                 sensor_raw_data[23:16] + sensor_raw_data[15:8]);
    }

    // 顯示 Transaction 詳細資訊的 Function (供 Generator / Driver 呼叫)
    function void display(string name);
        $display("[%s] RTOS Interrupt Offset: %0dus | Data: %h (RH:%0d.%0d, T:%0d.%0d)", 
                 name, rtos_interrupt_offset, sensor_raw_data,
                 sensor_raw_data[39:32], sensor_raw_data[31:24],
                 sensor_raw_data[23:16], sensor_raw_data[15:8]);
    endfunction
endclass


// ============================================================================
// [Phase 6.1 核心修復] 1-Wire 多節點二元樹搜尋模擬物件
// ============================================================================
class one_wire_node;
    rand bit [7:0]  family_code;
    rand bit [47:0] serial_num;
         bit [7:0]  crc8;

    constraint c_device {
        family_code == 8'h28; // 模擬主流 1-Wire 設備 (如 DS18B20) 的家族代碼
        serial_num  != 48'h0; // 避免全零無效序列號
    }

    // Maxim 1-Wire 標準硬件多項式計算: X^8 + X^5 + X^4 + 1 (反向多項式為 8'h8C)
    function bit [7:0] calc_crc8();
        bit [7:0] crc = 8'h0;
        bit [7:0] d;
        bit       mix;
        
        // 嚴格對齊 C 端的順序：先送 family_code (Byte 0)，再送 serial_num (Byte 1~6)
        bit [55:0] full_data = {serial_num, family_code};
        
        for (int i = 0; i < 7; i++) begin
            d = full_data[(i*8) +: 8];
            repeat(8) begin
                mix = (crc[0] ^ d[0]);
                crc = crc >> 1;
                if (mix) crc = crc ^ 8'h8C;
                d = d >> 1;
            end
        end
        return crc;
    endfunction

    // 隨機化完成後，自動呼叫硬體 CRC 計算
    function void post_randomize();
        crc8 = calc_crc8();
    endfunction

    // 組合出完整的 64-bit 唯一識別碼 (Rom ID)
    // 這樣拼接確保：[7:0] 是 Family Code, [55:8] 是 Serial Number, [63:56] 是 CRC8
    // 完美契合 C 語言韌體自低位元組往高位元組掃描的 LSB 物理特性！
    function bit [63:0] get_rom_id();
        return {crc8, serial_num, family_code};
    endfunction
    
    // 除錯列印資訊
    function void display_node(string name);
        $display("[%s] 1-Wire Node Active -> Rom ID: 64'h%h [FC: 8'h%h, Serial: 12'h%h, CRC: 8'h%h]", 
                 name, get_rom_id(), family_code, serial_num, crc8);
    endfunction
endclass
