`timescale 1ns/1ps
import bus_types::*;

module tb_top;

    // 1. 產生系統時脈 (System Clock Generator)
    bit clk;
    always #10 clk = ~clk; // 50MHz Clock (Period = 20ns)

    // 2. 實例化實體硬體訊號橋梁 (Interface Instantiation)
    bus_interface inf(clk);

    // 3. 綁定 SVA Asser Engine到Interface上 (Assertion Binding)
    bus_assertions u_assertions(.vif(inf.cb));

    // 4. 宣告驗證組件與環境變數
    bus_transaction tx;
    int bit_index;

    // 5. Simulation Control Loop
    initial begin
        $display("[TB_TOP] Initializing Pre-Silicon Functional Verification Pipeline.");
        
        // 初始化訊號線狀態
        inf.bus_data     = 1'b1;
        inf.rtos_tick    = 1'b0;
        inf.is_preempted = 1'b0;
        tx = new();

        // 模擬執行 5 個隨機Data Frames，觀察不同中斷點帶來的影響
        repeat(5) begin
            // 隨機化產生本次傳輸的資料與 FreeRTOS 中斷落點
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
                // 每一個 bit 開始前的 Low 電位 (50us)
                inf.bus_data = 1'b0;
                #(T_BIT_LOW * 1ns);

                // 決定此 bit 是 0 還是 1
                inf.bus_data = 1'b1;
                
                // 【核心模擬衝突點】
                // 如果隨機算出的中斷點（tx.rtos_interrupt_offset）剛好落在這個 bit 的傳輸區間內
                // 就讓 FreeRTOS Tick 中斷強行介入（rtos_tick 拉高），進而扯斷時序
                if (tx.rtos_interrupt_offset > 0 && tx.rtos_interrupt_offset < T_1_HIGH) begin
                    #(tx.rtos_interrupt_offset * 1ns);
                    inf.rtos_tick = 1'b1;     // FreeRTOS 中斷介入
                    inf.is_preempted = 1'b1;  // 設定硬體驗證旗標
                    #100000;                  // 模擬 Context Switch 造成的大延遲 (100us)
                    inf.rtos_tick = 1'b0;     // 離開中斷
                    break;                    // 此Frame已毀損，強制跳出
                end

                // 若沒發生衝突，則根據資料型態維持正常的 High 持續時間
                if (tx.sensor_raw_data[bit_index] == 1'b1)
                    #(T_1_HIGH * 1ns);
                else
                    #(T_0_HIGH * 1ns);
            end

            // 傳輸結束或毀損後的復歸狀態
            inf.bus_data     = 1'b1;
            inf.is_preempted = 1'b0;
            #500000; // Frames之間的間隔延遲
        end

        $display("[TB_TOP] Pre-Silicon Simulation Cycle Completed. Analyzing SVA coverage metrics.");
        $finish;
    end

endmodule
