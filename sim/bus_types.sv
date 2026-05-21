`timescale 1ns/1ps

package bus_types;
    // 定義 Single-wire 總線上的 Logic States
    typedef enum bit {
        LOGIC_0 = 1'b0,
        LOGIC_1 = 1'b1
    } bus_logic_e;

    // ========================================================================
    // DHT22 傳感器時序參數 (以微秒 us 為單位)
    // ========================================================================
    localparam int T_INIT_LOW   = 18000; 
    localparam int T_RESP_LOW   = 80;    
    localparam int T_RESP_HIGH  = 80;    
    localparam int T_BIT_LOW    = 50;    
    localparam int T_0_HIGH     = 26;    
    localparam int T_1_HIGH     = 70;    

    // ========================================================================
    // Phase 6.1: 1-Wire 多節點二元樹時序參數 (以微秒 us 為單位)
    // ========================================================================
    localparam int T_1W_RESET_LOW = 480;  // Master 發出 Reset 訊號
    localparam int T_1W_PRESENCE  = 100;  // Slave 回應 Presence 訊號
    localparam int T_1W_SLOT      = 65;   // 每個讀寫時隙（Slot）標準持續時間
    localparam int T_1W_SAMPLE    = 12;   // 讀取時隙的採樣窗口時間點
endpackage
