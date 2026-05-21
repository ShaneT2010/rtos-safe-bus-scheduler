`timescale 1ns/1ps

interface bus_interface(input bit clk);
    // 實體硬體訊號線（改為 tri 類型以支援多節點 Open-Drain 總線特徵）
    tri1 bus_data;    // Single-wire 雙向總線訊號，預設提升為高電位
    logic rtos_tick;  // 模擬 FreeRTOS 1ms 的作業系統心跳中斷

    // 驗證用內部狀態 Flags
    logic is_preempted; 

    // 用於 Testbench 驅動與採樣的專用訊號線（避免 inout 直接穿透 cb 引發 Race）
    logic bus_drv_en;
    logic bus_drv_val;

    // 實現 Open-Drain 驅動行為
    assign bus_data = (bus_drv_en) ? bus_drv_val : 1'bz;

    // 定義 Clocking Block：加入 Phase 6.1 所需的時序控制
    clocking cb @(posedge clk);
        default input #1ns output #1ns;
        input  bus_data;
        output bus_drv_en;
        output bus_drv_val;
        input  rtos_tick;
        input  is_preempted;
    endclocking

endinterface
