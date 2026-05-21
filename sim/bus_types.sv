`timescale 1ns/1ps

package bus_types;
    // 定義 Single-wire 總線上的Logic States & Timing（in 10^-6 s）
    typedef enum bit {
        LOGIC_0 = 1'b0,
        LOGIC_1 = 1'b1
    } bus_logic_e;

    // DHT22 傳感器時序參數 (以微秒 us 為單位)
    localparam int T_INIT_LOW  = 18000; // 啟動訊號：18ms
    localparam int T_RESP_LOW  = 80;    // 回應 Low 訊號：80us
    localparam int T_RESP_HIGH = 80;    // 回應 High 訊號：80us
    localparam int T_BIT_LOW   = 50;    // 每位元前的 Low 訊號：50us
    localparam int T_0_HIGH    = 26;    // 邏輯 0 的 High 持續時間：26us
    localparam int T_1_HIGH    = 70;    // 邏輯 1 的 High 持續時間：70us
endpackage
