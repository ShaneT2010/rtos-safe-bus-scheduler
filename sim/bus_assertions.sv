`timescale 1ns/1ps
import bus_types::*;

module bus_assertions(bus_interface.cb vif);

    // 50MHz 時脈下，一個 Cycle 是 20ns
    // 1 微秒 (1us) = 1000ns = 50 個 Clock Cycles (##50)
    
    // ------------------------------------------------------------------------
    // 【時域動態隔離旗標】
    // 由於我們在 tb_top.sv 中定義了 ow_bit_cnt，只要 ow_bit_cnt > 0 
    // 或者我們能辨識當前處於 Test Block B，就應該關閉 DHT22 檢查，開啟 1-Wire 檢查。
    // 這裡我們直接利用 tb_top 內部的節點啟動狀態，或者簡單透過暫存邏輯隔離。
    // ------------------------------------------------------------------------
    bit is_1wire_mode;
    assign is_1wire_mode = (tb_top.ow_bit_cnt > 0 || tb_top.read_phase_cmp != 1'b0);

    // ========================================================================
    // CHUNK A: Phase 1-5 專屬 — DHT22 單節點時序斷言 (修正 Cycle 計數與時脈域)
    // ========================================================================

    // DHT22 邏輯 0 High 持續時間: 26us (容許 24us~28us) -> 24*50=1200個 cycles 到 28*50=1400個 cycles
    property p_dht22_logic_0_width;
        @(posedge tb_top.clk) 
        ($rose(vif.bus_data) && !vif.is_preempted && !is_1wire_mode) |=-> 
            ##[1200:1400] $fell(vif.bus_data); 
    endproperty

    // DHT22 邏輯 1 High 持續時間: 70us (容許 68us~72us) -> 68*50=3400個 cycles 到 72*50=3600個 cycles
    property p_dht22_logic_1_width;
        @(posedge tb_top.clk)
        ($rose(vif.bus_data) && !vif.is_preempted && !is_1wire_mode) |=-> 
            ##[3400:3600] $fell(vif.bus_data);
    endproperty

    // DHT22 中斷碰撞偵測
    property p_rtos_collision_detect;
        @(posedge tb_top.clk)
        (vif.bus_data && $rose(vif.rtos_tick) && !is_1wire_mode) |=-> vif.is_preempted;
    endproperty


    // ========================================================================
    // CHUNK B: Phase 6.1 擴充 — 1-Wire 多節點二元樹物理層時序斷言
    // ========================================================================

    // 1-Wire 斷言 Rule 4：Master 發動 Reset Pulse 時，低電位必須持續約 480us (容許 450us~550us)
    // 450us * 50 = 22500 cycles, 550us * 50 = 27500 cycles
    property p_1wire_reset_pulse_width;
        @(posedge tb_top.clk)
        ($fell(vif.bus_data) && is_1wire_mode && tb_top.ow_bit_cnt == 0) |=-> 
            ##[22500:27500] $rose(vif.bus_data);
    endproperty

    // 1-Wire 斷言 Rule 5：安全防禦 — 二元樹運作期間，絕不允許任何 RTOS 中斷打穿臨界區
    property p_1wire_critical_zone_sanitizer;
        @(posedge tb_top.clk)
        is_1wire_mode |=-> (!vif.rtos_tick && !vif.is_preempted);
    endproperty


    // ========================================================================
    // 正式啟動 Assert / Cover Engines
    // ========================================================================
    
    // DHT22 啟動
    assert_dht22_logic_0: assert property (p_dht22_logic_0_width) 
        else $error("[SVA TIMING ERROR] Invalid DHT22 Logic '0' pulse width detected.");

    assert_dht22_logic_1: assert property (p_dht22_logic_1_width) 
        else $error("[SVA TIMING ERROR] Invalid DHT22 Logic '1' pulse width detected.");

    cover_rtos_collision: cover property (p_rtos_collision_detect)
        $warning("[SVA COLLISION WARNING] RTOS Tick interrupted active bus transaction! Data corrupted.");

    // 1-Wire 啟動
    assert_1wire_reset: assert property (p_1wire_reset_pulse_width)
        else $error("[SVA 1-WIRE ERROR] Master 1-Wire Reset low pulse width violates standard specs (480us).");

    assert_1wire_no_interrupt: assert property (p_1wire_critical_zone_sanitizer)
        else $fatal("[SVA FATAL SYSTEM BREAK] FreeRTOS Interrupt breached taskENTER_CRITICAL during 1-Wire SearchROM!");

endmodule
