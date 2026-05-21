`timescale 1ns/1ps
import bus_types::*;

module tb_top;

    // 1. 產生系統時脈 (System Clock Generator)
    bit clk;
    always #10 clk = ~clk; // 50MHz Clock (Period = 20ns)

    // 2. 實例化實體硬體訊號橋梁 (Interface Instantiation)
    bus_interface inf(clk);

    // 3. 綁定 SVA Assert Engine 到 Interface 上 (Assertion Binding)
    bus_assertions u_assertions(.vif(inf.cb));

    // 4. 宣告驗證組件與環境變數
    bus_transaction tx;         // [原本保留] DHT22 測試物件
    int bit_index;              // [原本保留] DHT22 位元索引
    
    one_wire_node nodes[3];     // [Phase 6.1 新增] 3 個隨機 1-Wire 節點
    bit [63:0] rom_matrix[3];   // [Phase 6.1 新增] 儲存 3 組 64-bit Rom ID
    int ow_bit_idx;             // [Phase 6.1 新增] 1-Wire 搜尋位元索引

    // 5. Simulation Control Loop
    initial begin
        $display("[TB_TOP] Initializing Pre-Silicon Functional Verification Pipeline.");
        
        // --- 初始化既有訊號線狀態 ---
        inf.bus_data     = 1'b1;
        inf.rtos_tick    = 1'b0;
        inf.is_preempted = 1'b0;
        
        // --- 初始化新增組件 ---
        tx = new();
        foreach(nodes[i]) begin
            nodes[i] = new();
            if (!nodes[i].randomize()) begin
                $error("[TB_ERROR] 1-Wire Node randomization failed!");
            end
            rom_matrix[i] = nodes[i].get_rom_id();
            nodes[i].display_node($sformatf("NODE_%0d_INIT", i));
        end

        // ====================================================================
        // 測試區塊 A：[原本保留] 模擬 5 個隨機 DHT22 Data Frames (含 RTOS 中斷搶佔測試)
        // ====================================================================
        $display("[TB_TOP] Starting Test Block A: DHT22 Single-Node RTOS Jitter Injection.");
        repeat(5) begin
            if (!tx.randomize()) begin
                $error("[TB_ERROR] Transaction randomization failed!");
            end
            tx.display("GENERATOR");

            // --- 模擬 DHT22 傳輸時序開始 ---
            #1000; 
            inf.bus_data = 1'b0; // 觸發啟動訊號 (Start Signal)
            #(T_INIT_LOW * 1ns);
            
            inf.bus_data = 1'b1; // 釋放總線，等待回應
            #(T_RESP_LOW * 1ns);
            inf.bus_data = 1'b0;
            #(T_RESP_HIGH * 1ns);

            // 開始傳送 40-bit 隨機資料
            for (bit_index = 39; bit_index >= 0; bit_index--) begin
                inf.bus_data = 1'b0;
                #(T_BIT_LOW * 1ns);

                inf.bus_data = 1'b1;
                
                // 【核心模擬衝突點】
                if (tx.rtos_interrupt_offset > 0 && tx.rtos_interrupt_offset < T_1_HIGH) begin
                    #(tx.rtos_interrupt_offset * 1ns);
                    inf.rtos_tick = 1'b1;     // FreeRTOS 中斷介入
                    inf.is_preempted = 1'b1;  // 設定硬體驗證旗標
                    #100000;                  // 模擬 Context Switch 造成的大延遲 (100us)
                    inf.rtos_tick = 1'b0;     // 離開中斷
                    break;                    // 此Frame已毀損，強制跳出
                end

                if (tx.sensor_raw_data[bit_index] == 1'b1)
                    #(T_1_HIGH * 1ns);
                else
                    #(T_0_HIGH * 1ns);
            end

            inf.bus_data     = 1'b1;
            inf.is_preempted = 1'b0;
            #500000; // Frames之間的間隔延遲
        end

        // ====================================================================
        // 測試區塊 B：[Phase 6.1 新增] 模擬 1-Wire 多節點二元樹搜尋匯流排碰撞
        // ====================================================================
        $display("[TB_TOP] Starting Test Block B: 1-Wire Multi-Drop Search ROM Arbitration.");
        
        // 模擬韌體發動 Search ROM 命令 (0xF0) 後的 64 次位元掃描流程
        // 這裡我們連續跑 3 輪完整的 Search ROM，直到 3 個虛擬節點全部被韌體分離出來
        repeat(3) begin
            $display("[TB_TOP] New Search ROM Cycle Initiated by Firmware.");
            
            // 根據 1-Wire 協定，64 位元中每一位元包含：[讀取原位元] 與 [讀取反轉位元]
            for (ow_bit_idx = 0; ow_bit_idx < 64; ow_bit_idx++) begin
                bit bus_bit = 1'b1;
                bit bus_cmp_bit = 1'b1;

                // 模擬 Open-Drain 的 Wired-AND 物理特性：
                // 只要任何一個存活節點的當前 bit 為 0，總線就會被拉低
                foreach(rom_matrix[i]) begin
                    if (rom_matrix[i][ow_bit_idx] == 1'b0)     bus_bit = 1'b0;
                    if (rom_matrix[i][ow_bit_idx] == 1'b1)     bus_cmp_bit = 1'b0; // 反轉位元
                end

                // --- 階段 1：驅動原位元到總線上，維持 15us 供韌體採樣 ---
                inf.bus_data = bus_bit;
                #15000; 

                // --- 階段 2：驅動反轉位元到總線上，維持 15us 供韌體採樣 ---
                inf.bus_data = bus_cmp_bit;
                #15000;

                // --- 階段 3：釋放總線，等待韌體回寫它選擇的路徑位元 (Direction Bit) ---
                inf.bus_data = 1'b1; 
                #30000; // 期待韌體在主控端驅動選擇的分支
            end
            
            #1000000; // 兩次 Search ROM 掃描之間的匯流排沉澱時間 (1ms)
        end

        $display("[TB_TOP] Pre-Silicon Simulation Cycle Completed. Analyzing SVA coverage metrics.");
        $finish;
    end

endmodule
