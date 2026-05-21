import bus_types::*;

class bus_transaction;
    // 隨機變數：模擬 FreeRTOS 核心Interrupt發生的時間點 (單位: us)
    // FreeRTOS Tick Rate = 1000Hz (1ms), 故範圍在 0 到 1000 微秒之間
    rand int rtos_interrupt_offset;
    
    // 隨機變數：40-bit 的傳感器Raw Data
    rand bit [39:0] sensor_raw_data;

    // 限制區塊 (Constraint Blocks)：定義合法的隨機範圍
    constraint c_rtos_timing {
        rtos_interrupt_offset inside {[0:1000]}; 
    }

    constraint c_sensor_data {
        // 限制濕度與溫度在合理物理區間，避免無意義的隨機訊號
        sensor_raw_data[39:24] inside {[0:1000]}; // 濕度 0.0% - 100.0%
        sensor_raw_data[23:8]  inside {[0:500]};  // 溫度 0.0°C - 50.0°C
        // Checksum 自動計算欄位
        sensor_raw_data[7:0] == (sensor_raw_data[39:32] + sensor_raw_data[31:24] + 
                                 sensor_raw_data[23:16] + sensor_raw_data[15:8]);
    }

    // 顯示 Transaction 詳細資訊的 Function
    function void display(string name);
        $display("[%s] RTOS Interrupt Offset: %0dus | Data: %h (RH:%0d.%0d, T:%0d.%0d)", 
                 name, rtos_interrupt_offset, sensor_raw_data,
                 sensor_raw_data[39:32], sensor_raw_data[31:24],
                 sensor_raw_data[23:16], sensor_raw_data[15:8]);
    endfunction
endclass
