#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Preemptive RTOS Window Scheduling Project - Telemetry Data Verification Bridge
Author: ShaneT2010
Description: Logs raw UART data streams from STM32, performs host-side checksum
             evaluation, and quantifies context-switch collision impacts.
"""

import time
import sys

# Serial Port解析類別: Design Verification Bridge
class DVBridge:
    def __init__(self, port_name="COM3"):
        self.port_name = port_name
        self.total_frames = 0
        self.corrupted_frames = 0
        print(f"[DV_BRIDGE] Initializing telemetry monitor on port {self.port_name}...")

    def verify_packet(self, raw_hex_str, timestamp_offset_us):
        """
        驗證從 MCU 傳回的 40-bit 原始資料
        格式預期：5位元組的十六進制字串，例如 "0285014CDA"
        """
        self.total_frames += 1
        
        if len(raw_hex_str) != 10:
            print(f"[DV_WARN] Malformed packet packet received: {raw_hex_str}")
            return False
            
        try:
            # 將十六進制字串轉換為位元組陣列 (5 Bytes)
            data_bytes = bytes.fromhex(raw_hex_str)
            
            # 拆解資料結構 (呼應 Phase 1 bus_transaction 與 Phase 2 HAL)
            humidity_high = data_bytes[0]
            humidity_low  = data_bytes[1]
            temp_high     = data_bytes[2]
            temp_low      = data_bytes[3]
            checksum_rcv  = data_bytes[4]
            
            # Checksum Calculation
            checksum_calc = (humidity_high + humidity_low + temp_high + temp_low) & 0xFF
            
            # 計算物理數值 (攝氏溫度與相對濕度)
            humidity = ((humidity_high << 8) | humidity_low) / 10.0
            temperature = (((temp_high & 0x7F) << 8) | temp_low) / 10.0
            if temp_high & 0x80:
                temperature = -temperature # 負溫處理
                
            is_valid = (checksum_rcv == checksum_calc)
            
            print(f"\n--- [Frame #{self.total_frames:03d} Timestamp: +{timestamp_offset_us}us] ---")
            print(f"  Raw Payload : 0x{raw_hex_str}")
            print(f"  Calculated  : RH={humidity}% | Temp={temperature}°C")
            
            if is_valid:
                print("  Status      : PASS [✓ Checksum Valid]")
            else:
                self.corrupted_frames += 1
                print(f"  Status      : FAIL [✗ Checksum Mismatch! Expected 0x{checksum_calc:02X}, Got 0x{checksum_recv:02X}]")
                print(f"  Analysis    : Microsecond jitter telemetry points to kernel preemption interference.")
                
            return is_valid
            
        except Exception as e:
            print(f"[DV_ERROR] Protocol parsing exception: {str(e)}")
            return False

    def print_summary(self):
        if self.total_frames == 0:
            return
        drop_rate = (self.corrupted_frames / self.total_frames) * 100
        print("\n========================================================")
        print("                TELEMETRY PROFILE SUMMARY               ")
        print("========================================================")
        print(f" Total Ingested Frames : {self.total_frames}")
        print(f" Verified Legal Frames : {self.total_frames - self.corrupted_frames}")
        print(f" Corrupted Data Frames : {self.corrupted_frames}")
        print(f" Packet Drop Rate (PDR): {drop_rate:.2f}%")
        print("========================================================")

# 測試腳本：模擬硬體傳回的序列流，包含正常封包與中斷時序導致的壞封包
if __name__ == "__main__":
    bridge = DVBridge(port_name="VIRTUAL_COM")
    
    # 模擬場景 1：正常傳輸
    bridge.verify_packet("0285014CDA", timestamp_offset_us=240)
    time.sleep(0.1)
    
    # 模擬場景 2：中斷介入，導致最後一個 Byte (Checksum) 採樣錯誤變成了 0x00
    bridge.verify_packet("0285014C00", timestamp_offset_us=850)
    time.sleep(0.1)
    
    # 模擬場景 3：正常傳輸
    bridge.verify_packet("028B0152DF", timestamp_offset_us=120)
    
    # 輸出統計結果
    bridge.print_summary()
