`timescale 1ns/1ps
import bus_types::*;

module tb_top;

    // ========================================================================
    // 1. 系統時脈與第一版原始硬體介面 (100% 繼承 Phase 1-5，絕不動它)
    // ========================================================================
    bit clk;
    always #10 clk = ~clk; // 50MHz Clock (Period = 20ns)

    // 實例化實體硬體訊號橋梁 (與第一版完全對齊)
    bus_interface inf(clk);

    // 綁定 SVA Assert Engine 到 Interface 上
    bus_assertions u_assertions(.vif(inf.cb));

    // ========================================================================
    // 2. 宣告驗證組件與環境變數 (完好保留第一版變數，並乾淨擴充 Phase 6.1)
    // ========================================================================
    // [Phase 1-5 保留] DHT22 驗證組件
    bus_transaction tx;         
    int bit_index;              
    
    // [Phase 6.1 擴充] 1-Wire 多節點驗證組件
    one_wire_node nodes[3];     // 3 個隨機 1-Wire 虛擬感測器
    bit [63:0] rom_matrix[3];   // 儲存 3 組 64-bit Rom ID
    bit        node_active[3];  // 節點存活狀態旗標 (用於二元樹仲裁淘汰)
    int        ow_bit_cnt;      // 當前搜尋的位元計數器 (0 到 63)
    bit        read_phase_cmp;  // 標記目前是讀取[原位元]還是[反轉位元]

    // ========================================================================
    // 3. DPI-C 介面宣告：區分 DHT22 與 1-Wire，不讓名稱與邏輯打架
    // ========================================================================
    
    // --- (A) Phase 1-5 的 DHT22 DPI-C 接口 (維持原有名稱，讓原本的驅動可以跑) ---
    export "DPI-C" function dht22_read_bit;
    
    function bit dht22_read_bit();
        // 直接回傳第一版 Interface 上的真實總線狀態
        return inf.bus_data;
    endfunction

    // --- (B) Phase 6.1 的 1-Wire 全新 DPI-C 接口 (使用獨立名稱 ow_xxx，防止污染既有程式碼) ---
    export "DPI-C" task ow_bus_reset;
    export "DPI-C" function ow_read_bit;
    export "DPI-C" task ow_write_bit;
    
    // 解決時間軸脫節：將 SV 的時間延遲導出給 C 韌體呼叫
    export "DPI-C" task sv_delay_us;
    task sv_delay_us(int us);
        #(us * 1000); // 微秒轉奈秒
    endtask

    // 解鎖軟硬體死結：引入 C 韌體搜尋的執行進入點
    import "DPI-C" context task c_run_one_wire_test();

    // 1-Wire 總線重置：重置時所有總線節點重新復活，準備競爭
    task ow_bus_reset();
        $display("[DPI-C 1-WIRE] Master issued Bus Reset. Regenerating node grid.");
        foreach(node_active[i]) node_active[i] = 1'b1;
        ow_bit_cnt = 0;
        read_phase_cmp = 1'b0;
        
        // 模擬 1-Wire 實體重置時序，透過第一版的 inf.bus_data 表現波形
        inf.bus_data = 1'b0; #480000; // 拉低 480us
        inf.bus_data = 1'b1; #70000;  // 釋放總線等待 Presence
        inf.bus_data = 1'b0; #120000; // 模擬 Slave 設備集體拉低響應 Presence Pulse
        inf.bus_data = 1'b1; #290000; // 釋放
    endtask

    // 1-Wire 位元讀取：根據目前存活節點的 ROM ID，動態回傳 Wired-AND 疊加結果
    function bit ow_read_bit();
        bit bus_state = 1'b1; // 上拉電阻預設為高電位
        int byte_idx = ow_bit_cnt / 8;
        int bit_shift = ow_bit_cnt % 8;

        foreach(nodes[i]) begin
            if (node_active[i]) begin
                bit current_rom_bit = (rom_matrix[i][byte_idx] >> bit_shift) & 1'b1;
                
                if (!read_phase_cmp) begin
                    // 讀取原位元：若元件該位元為 0，則強行拉低總線
                    if (current_rom_bit == 1'b0) bus_state = 1'b0;
                end else begin
                    // 讀取反轉位元：若元件該位元為 1 (反轉後為 0)，則強行拉低總線
                    if (current_rom_bit == 1'b1) bus_state = 1'b0;
                end
            end
        end
        
        // 狀態機步進：交替切換「原位元相位」與「反轉位元相位」
        if (!read_phase_cmp) begin
            read_phase_cmp = 1'b1; 
        end else begin
            read_phase_cmp = 1'b0; 
        end
        
        return bus_state;
    endfunction

    // 1-Wire 寫入位元：實施真正的二元樹實體路徑淘汰 (Arbitration)
    task ow_write_bit(bit b);
        int byte_idx = ow_bit_cnt / 8;
        int bit_shift = ow_bit_cnt % 8;

        // 核心仲裁：Master 選定的分支路徑如果跟節點本身的 ROM 位元不符，該節點當場出局！
        foreach(nodes[i]) begin
            if (node_active[i]) begin
                bit current_rom_bit = (rom_matrix[i][byte_idx] >> bit_shift) & 1'b1;
                if (current_rom_bit != b) begin
                    node_active[i] = 1'b0; // 淘汰，本輪搜尋不再響應總線
                    $display("[DPI-C 1-WIRE] Node %0d Disqualified at Bit %0d (Node expected %b, Master drove %b)", i, ow_bit_cnt+1, current_rom_bit, b);
                end
            end
        end

        // 位元生命週期結束，推進到下一個 bit，重置讀取相位
        ow_bit_cnt++;
        read_phase_cmp = 1'b0;
    endtask

    // ========================================================================
    // 4. Simulation Control Loop (百分之百保留第一版 Block A，並完美銜接 Block B)
    // ========================================================================
    initial begin
        $display("[TB_TOP] Initializing Pre-Silicon Functional Verification Pipeline.");
        
        // --------------------------------------------------------------------
        // [100% 第一版原始狀態初始化]
        // --------------------------------------------------------------------
        inf.bus_data     = 1'b1;
        inf.rtos_tick    = 1'b0;
        inf.is_preempted = 1'b0;
        tx = new();

        // --------------------------------------------------------------------
        // [Phase 6.1 1-Wire 節點隨機化初始化]
        // --------------------------------------------------------------------
        foreach(nodes[i]) begin
            nodes[i] = new();
            if (!nodes[i].randomize()) begin
                $error("[TB_ERROR] 1-Wire Node randomization failed!");
            end
            rom_matrix[i] = nodes[i].get_rom_id();
            nodes[i].display_node($sformatf("NODE_%0d_INIT", i));
        end

        // --------------------------------------------------------------------
        // 測試區塊 A：[第一版原封不動保留] DHT22 單節點 RTOS 中斷搶佔測試
        // --------------------------------------------------------------------
        $display("[TB_TOP] Starting Test Block A: DHT22 Single-Node RTOS Jitter Injection.");
        repeat(5) begin
            if (!tx.randomize()) begin
                $error("[TB_ERROR] Transaction randomization failed!");
            end
            tx.display("GENERATOR");

            // --- 模擬第一版 DHT22 傳輸時序 ---
            #1000; 
            inf.bus_data = 1'b0; // 觸發啟動訊號 (Start Signal)
            #(T_INIT_LOW * 1ns);
            
            inf.bus_data = 1'b1; // 釋放總線，等待回應
            #(T_RESP_LOW * 1ns);
            inf.bus_data = 1'b0;
            #(T_RESP_HIGH * 1ns);

            for (bit_index = 39; bit_index >= 0; bit_index--) begin
                inf.bus_data = 1'b0;
                #(T_BIT_LOW * 1ns);

                inf.bus_data = 1'b1;
                
                // 【核心模擬衝突點】
                if (tx.rtos_interrupt_offset > 0 && tx.rtos_interrupt_offset < T_1_HIGH) begin
                    #(tx.rtos_interrupt_offset * 1ns);
                    inf.rtos_tick = 1'b1;     
                    inf.is_preempted = 1'b1;  
                    #100000;                  
                    inf.rtos_tick = 1'b0;     
                    break;                    
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

        // --------------------------------------------------------------------
        // 測試區塊 B：[Phase 6.1 銜接點] 啟動 1-Wire 嵌入式核心二元樹搜尋
        // --------------------------------------------------------------------
        $display("[TB_TOP] Starting Test Block B: Launching C-Embedded 1-Wire SearchROM Engine.");
        
        // 核心修正：不再傻傻盲等 #1000000，主動把權限交給 C 語言的主測試執行緒
        c_run_one_wire_test();
        
        $display("[TB_TOP] Pre-Silicon Simulation Cycle Completed. Analyzing SVA coverage metrics.");
        $finish;
    end

endmodule
