`timescale 1ns/1ps

interface bus_interface(input bit clk);
    // 實體硬體訊號線
    logic bus_data;   // Single-wire 雙向總線訊號
    logic rtos_tick;  // 模擬 FreeRTOS 1ms 的作業系統心跳中斷

    // 驗證用內部狀態Flags（用於同步 Driver 與 Monitor 的行為）
    logic is_preempted; 

    // 定義 Clocking Block：確保驗證環境在Clock Edge採樣，避免模擬中的 Race Condition
    clocking cb @(posedge clk);
        default input #1ns output #1ns;
        inout bus_data;
        input rtos_tick;
        input is_preempted;
    endclocking

endinterface
