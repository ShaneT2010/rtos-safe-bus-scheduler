`timescale 1ns/1ps
import bus_types::*;

module bus_assertions(bus_interface.cb vif);

    // ---------------------------------------------------------
  // Assertion Rule 1：邏輯 0 的 High 持續時間必須為 26微秒 (容許 ±2us 物理Jitter)
    // ---------------------------------------------------------
    property p_logic_0_width;
        // 當偵測到總線從 Low 變 High，且當前並未被 FreeRTOS 中斷搶占時
        ($rose(vif.bus_data) && !vif.is_preempted) |=-> 
            ##[24:28] $fell(vif.bus_data); 
    endproperty

    // ---------------------------------------------------------
    // Assertion Rule 2：邏輯 1 的 High 持續時間必須為 70微秒 (容許 ±2us 物理Jitter)
    // ---------------------------------------------------------
    property p_logic_1_width;
        ($rose(vif.bus_data) && !vif.is_preempted) |=-> 
            ##[68:72] $fell(vif.bus_data);
    endproperty

    // ---------------------------------------------------------
    // Assertion Rule 3：若傳輸中發生 FreeRTOS 有Interrupt Preemption，資料必定毀損，觸發 Warning
    // ---------------------------------------------------------
    property p_rtos_collision_detect;
        // 在傳輸過程中（bus_data 為 High 時），若 rtos_tick 突然拉高（中斷發生）
        (vif.bus_data && $rose(vif.rtos_tick)) |=-> vif.is_preempted;
    endproperty

    // 正式啟動Assert Engines
    assert_logic_0: assert property (p_logic_0_width) 
        else $error("[SVA TIMING ERROR] Invalid Logic '0' pulse width detected.");

    assert_logic_1: assert property (p_logic_1_width) 
        else $error("[SVA TIMING ERROR] Invalid Logic '1' pulse width detected.");

    cover_rtos_collision: cover property (p_rtos_collision_detect)
        $warning("[SVA COLLISION WARNING] RTOS Tick interrupted active bus transaction! Data corrupted.");

endmodule
