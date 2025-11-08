#!/usr/bin/env python3
"""
ECU_011 Simulator
Simulates Zone ECU sending VCI to Zonal Gateway
"""

import socket
import struct
import time
import sys

class ECU_011_Simulator:
    """ECU_011 Simulator - Sends VCI to Zonal Gateway"""
    
    # VCI Message Magic Number
    VCI_MAGIC = 0x56434921  # "VCI!"
    
    def __init__(self, zg_ip='192.168.1.10', zg_port=13400):
        self.zg_ip = zg_ip
        self.zg_port = zg_port
        
        # ECU_011 VCI Information (matches doip_types.h)
        self.ecu_id = "ECU_011"
        self.sw_version = "0.0.0"
        self.hw_version = "0.0.0"
        self.serial_num = "011000001"
        
        # Create UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
    def create_vci_message(self):
        """Create VCI message payload"""
        # Message format:
        # Magic Number (4 bytes) + ECU ID (16) + SW Ver (8) + HW Ver (8) + Serial (16)
        
        message = struct.pack('!I', self.VCI_MAGIC)
        
        # Pad strings to fixed length
        ecu_id_bytes = self.ecu_id.encode('ascii').ljust(16, b'\x00')
        sw_ver_bytes = self.sw_version.encode('ascii').ljust(8, b'\x00')
        hw_ver_bytes = self.hw_version.encode('ascii').ljust(8, b'\x00')
        serial_bytes = self.serial_num.encode('ascii').ljust(16, b'\x00')
        
        message += ecu_id_bytes + sw_ver_bytes + hw_ver_bytes + serial_bytes
        
        return message
    
    def send_vci(self):
        """Send VCI to Zonal Gateway"""
        message = self.create_vci_message()
        
        try:
            self.sock.sendto(message, (self.zg_ip, self.zg_port))
            print(f"[ECU_011] VCI sent to {self.zg_ip}:{self.zg_port}")
            print(f"  ECU ID: {self.ecu_id}")
            print(f"  SW Version: {self.sw_version}")
            print(f"  HW Version: {self.hw_version}")
            print(f"  Serial Number: {self.serial_num}")
            return True
        except Exception as e:
            print(f"[ECU_011] Failed to send VCI: {e}")
            return False
    
    def run(self, delay_seconds=3):
        """Run simulator - send VCI after delay"""
        print("=" * 60)
        print("ECU_011 Simulator Started")
        print(f"Target: {self.zg_ip}:{self.zg_port}")
        print(f"Sending VCI in {delay_seconds} seconds...")
        print("=" * 60)
        
        time.sleep(delay_seconds)
        
        if self.send_vci():
            print("[ECU_011] VCI transmission successful")
        else:
            print("[ECU_011] VCI transmission failed")
        
        self.sock.close()


def main():
    """Main function"""
    # Default configuration
    zg_ip = '192.168.1.10'
    zg_port = 13400
    delay = 5  # Wait 5 seconds before sending
    
    # Parse command line arguments
    if len(sys.argv) > 1:
        zg_ip = sys.argv[1]
    if len(sys.argv) > 2:
        delay = int(sys.argv[2])
    
    # Create and run simulator
    ecu = ECU_011_Simulator(zg_ip, zg_port)
    ecu.run(delay)


if __name__ == '__main__':
    main()

