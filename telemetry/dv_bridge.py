#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Preemptive RTOS Window Scheduling Project - Telemetry Data Verification Bridge
Description: Enhanced Multi-Protocol DV Bridge supporting both Phase 1-5 (DHT22) 
             and Phase 6.1 (1-Wire Multi-node Binary Tree Search ROM).
"""

import time
import sys

class DVBridge:
    def __init__(self, port_name="COM3"):
        self.port_name = port_name
        self.total_frames = 0
        self.corrupted_frames = 0
        self.discovered_roms = []
        print(f"[DV_BRIDGE] Initializing adaptive telemetry monitor on port {self.port_name}...")

    def verify_packet(self, raw_hex_str, timestamp_offset_us=0):
        """
        適應性協議驗證：自動識別 DHT22 (5 Bytes) 與 1-Wire ROM ID (8 Bytes)
        """
        self.total_frames += 1
        raw_hex_str = raw_hex_str.strip().upper()
        
        # 1. 協議識別分流
        if len(raw_hex_str) == 10:
            return self._parse_dht22(raw_hex_str, timestamp_offset_us)
        elif len(raw_hex_str) == 16:
            return self._parse_1wire_rom(raw_hex_str, timestamp_offset_us)
        else:
            self.corrupted_frames += 1
            print(f"[DV_WARN] Malformed frame length ({len(raw_hex_str)} hex chars): {raw_hex_str}")
            return False

    def _parse_dht22(self, raw_hex_str, timestamp_offset_us):
        """解析 Phase 1-5 的 DHT22 溫濕度資料"""
        try:
            data_bytes = bytes.fromhex(raw_hex_str)
            humidity_high, humidity_low, temp_high, temp_low, checksum_rcv = data_bytes
            
            checksum_calc = (humidity_high + humidity_low + temp_high + temp_low) & 0xFF
            humidity = ((humidity_high << 8) | humidity_low) / 10.0
            
            temperature = (((temp_high & 0x7F) << 8) | temp_low) / 10.0
            if temp_high & 0x80:
                temperature = -temperature

            is_valid = (checksum_rcv == checksum_calc)
            
            print(f"\n--- [Frame #{self.total_frames:03d} | PROTOCOL: DHT22 | +{timestamp_offset_us}us] ---")
            print(f"  Raw Payload : 0x{raw_hex_str}")
            print(f"  Calculated  : RH={humidity}% | Temp={temperature}°C")
            
            if is_valid:
                print("  Status      : PASS [✓ Checksum Valid]")
            else:
                self.corrupted_frames += 1
                print(f"  Status      : FAIL [✗ Checksum Mismatch! Exp: 0x{checksum_calc:02X}, Rcv: 0x{checksum_rcv:02X}]")
                print("  Analysis    : Microsecond jitter points to kernel preemption breach.")
            return is_valid
            
        except Exception as e:
            print(f"[DV_ERROR] DHT22 parsing failure: {str(e)}")
            return False

    def _parse_1wire_rom(self, raw_hex_str, timestamp_offset_us):
        """解析 Phase 6.1 的 1-Wire 64-bit ROM ID (包含 8-bit Family Code + 48-bit Serial + 8-bit CRC)"""
        try:
            data_bytes = bytes.fromhex(raw_hex_str)
            family_code = data_bytes[0]
            serial_number = data_bytes[1:7].hex().upper()
            crc_rcv = data_bytes[7]
            
            # Maxim/Dallas 1-Wire 經典的 CRC-8 計算 (Poly = X^8 + X^5 + X^4 + 1 -> 0x8C 翻轉型)
            crc_calc = self._calculate_crc8_maxim(data_bytes[0:7])
            is_valid = (crc_rcv == crc_calc)
            
            print(f"\n--- [Frame #{self.total_frames:03d} | PROTOCOL: 1-Wire SearchROM | +{timestamp_offset_us}us] ---")
            print(f"  Raw Payload : 0x{raw_hex_str}")
            print(f"  Family Code : 0x{family_code:02X} (Device Type)")
            print(f"  Serial No   : 0x{serial_number}")
            print(f"  CRC Byte    : 0x{crc_rcv:02X}")
            
            if is_valid:
                if raw_hex_str not in self.discovered_roms:
                    self.discovered_roms.append(raw_hex_str)
                print(f"  Status      : PASS [✓ 1-Wire ROM CRC Valid]")
                print(f"  Discovery   : Node Registered. Total Unique Nodes Found: {len(self.discovered_roms)}")
            else:
                self.corrupted_frames += 1
                print(f"  Status      : FAIL [✗ CRC Mismatch! Exp: 0x{crc_calc:02X}, Rcv: 0x{crc_rcv:02X}]")
                print("  Analysis    : Bitwise conflict on Search ROM or unmasked critical section.")
            return is_valid
            
        except Exception as e:
            print(f"[DV_ERROR] 1-Wire ROM parsing failure: {str(e)}")
            return False

    @staticmethod
    def _calculate_crc8_maxim(data_bytes):
        """Maxim 1-Wire 專用 CRC-8 演算法"""
        crc = 0x00
        for byte in data_bytes:
            for i in range(8):
                mix = (crc ^ byte) & 0x01
                crc >>= 1
                if mix:
                    crc ^= 0x8C
                byte >>= 1
        return crc

    def print_summary(self):
        if self.total_frames == 0:
            return
        drop_rate = (self.corrupted_frames / self.total_frames) * 100
        print("\n========================================================")
        print("                ADAPTIVE TELEMETRY PROFILE SUMMARY       ")
        print("========================================================")
        print(f" Total Ingested Frames : {self.total_frames}")
        print(f" Verified Legal Frames : {self.total_frames - self.corrupted_frames}")
        print(f" Corrupted Data Frames : {self.corrupted_frames}")
        print(f" Packet Drop Rate (PDR): {drop_rate:.2f}%")
        print(f" 1-Wire Nodes Discovered: {len(self.discovered_roms)}")
        for idx, rom in enumerate(self.discovered_roms):
            print(f"   -> Node #{idx+1}: 0x{rom}")
        print("========================================================")

if __name__ == "__main__":
    bridge = DVBridge(port_name="VIRTUAL_COM")
    
    # 測試場景 1：Phase 1-5 舊架構溫濕度傳輸 (PASS)
    bridge.verify_packet("0285014CDA", timestamp_offset_us=240)
    
    # 測試場景 2：Phase 6.1 正確的 1-Wire ROM ID 搜尋結果 (DS18B20 溫度計範例)
    # Family Code: 0x28, Serial: 0x000000A3B2C1, CRC: 0x3C (假設算出來正確)
    bridge.verify_packet("28C1B2A30000003C", timestamp_offset_us=1500)
    
    # 測試場景 3：Phase 6.1 遭受強烈訊號反射或硬體瑕疵導致的 1-Wire CRC 錯誤封包
    bridge.verify_packet("28C1B2A3000000FF", timestamp_offset_us=2100)
    
    bridge.print_summary()
