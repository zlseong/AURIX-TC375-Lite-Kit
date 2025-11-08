#!/usr/bin/env python3
"""
VMG Simulator Tester
Simple DoIP client to test VMG Simulator
"""

import socket
import struct
import time

class SimpleDoIPClient:
    """Simple DoIP Client for testing"""
    
    def __init__(self, server_ip, server_port=13400):
        self.server_ip = server_ip
        self.server_port = server_port
        self.sock = None
    
    def connect(self):
        """Connect to VMG Simulator"""
        print(f"🔌 Connecting to VMG at {self.server_ip}:{self.server_port}...")
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.server_ip, self.server_port))
        print(f"✅ Connected!\n")
    
    def send_routing_activation(self):
        """Send Routing Activation Request"""
        print("📤 Sending Routing Activation Request...")
        
        # DoIP Header
        protocol_version = 0x02
        inverse_version = 0xFD
        payload_type = 0x0005  # Routing Activation Request
        
        # Payload: Source Address (2) + Activation Type (1) + Reserved (4)
        source_address = 0x0E00  # Tester address
        activation_type = 0x00   # Default activation
        reserved = 0x00000000
        
        payload = struct.pack('!HBL', source_address, activation_type, reserved)
        payload_length = len(payload)
        
        # Complete message
        header = struct.pack('!BBHI', protocol_version, inverse_version, payload_type, payload_length)
        message = header + payload
        
        self.sock.send(message)
        print(f"   Sent {len(message)} bytes")
        
        # Receive response
        response = self.sock.recv(1024)
        if len(response) >= 8:
            _, _, resp_type, resp_len = struct.unpack('!BBHI', response[:8])
            print(f"📥 Received response: Type=0x{resp_type:04x}, Length={resp_len}")
            
            if resp_type == 0x0006:  # Routing Activation Response
                if len(response) >= 13:
                    vmg_addr, client_addr, response_code = struct.unpack('!HHB', response[8:13])
                    print(f"   VMG Address: 0x{vmg_addr:04x}")
                    print(f"   Client Address: 0x{client_addr:04x}")
                    print(f"   Response Code: 0x{response_code:02x}")
                    
                    if response_code == 0x10:
                        print(f"✅ Routing Activation SUCCESS!\n")
                        return True
                    else:
                        print(f"❌ Routing Activation FAILED: 0x{response_code:02x}\n")
                        return False
        return False
    
    def send_alive_check_response(self):
        """Send Alive Check Response"""
        print("📤 Sending Alive Check Response...")
        
        protocol_version = 0x02
        inverse_version = 0xFD
        payload_type = 0x0008  # Alive Check Response
        
        source_address = 0x0E00
        payload = struct.pack('!H', source_address)
        payload_length = len(payload)
        
        header = struct.pack('!BBHI', protocol_version, inverse_version, payload_type, payload_length)
        message = header + payload
        
        self.sock.send(message)
        print(f"   Sent {len(message)} bytes\n")
    
    def send_zone_status_report(self):
        """Send Zone Status Report (simulating Zonal Gateway)"""
        print("📤 Sending Zone Status Report...")
        
        protocol_version = 0x02
        inverse_version = 0xFD
        payload_type = 0x9001  # Zone Status Report (custom)
        
        # Payload: Zone Count + (Zone ID + Health + DTC + Battery + Temp) * N
        zone_count = 3
        
        # Zone 1: Body
        zone1_id = 1
        zone1_health = 0  # OK
        zone1_dtc = 0
        zone1_battery = 130  # 13.0V
        zone1_temp = 65  # 25°C (offset by 40)
        
        # Zone 2: ADAS
        zone2_id = 2
        zone2_health = 1  # WARNING
        zone2_dtc = 2
        zone2_battery = 125  # 12.5V
        zone2_temp = 70  # 30°C
        
        # Zone 3: Telematics
        zone3_id = 3
        zone3_health = 0  # OK
        zone3_dtc = 0
        zone3_battery = 128  # 12.8V
        zone3_temp = 68  # 28°C
        
        payload = struct.pack('!B', zone_count)
        payload += struct.pack('!BBBBB', zone1_id, zone1_health, zone1_dtc, zone1_battery, zone1_temp)
        payload += struct.pack('!BBBBB', zone2_id, zone2_health, zone2_dtc, zone2_battery, zone2_temp)
        payload += struct.pack('!BBBBB', zone3_id, zone3_health, zone3_dtc, zone3_battery, zone3_temp)
        
        payload_length = len(payload)
        
        header = struct.pack('!BBHI', protocol_version, inverse_version, payload_type, payload_length)
        message = header + payload
        
        self.sock.send(message)
        print(f"   Sent {len(message)} bytes")
        print(f"   Zone 1 (Body): OK, 0 DTCs, 13.0V, 25°C")
        print(f"   Zone 2 (ADAS): WARNING, 2 DTCs, 12.5V, 30°C")
        print(f"   Zone 3 (Telematics): OK, 0 DTCs, 12.8V, 28°C\n")
    
    def wait_for_alive_check(self, timeout=10):
        """Wait for Alive Check Request from VMG"""
        print(f"⏳ Waiting for Alive Check from VMG (timeout: {timeout}s)...")
        
        self.sock.settimeout(timeout)
        try:
            data = self.sock.recv(1024)
            if len(data) >= 8:
                _, _, msg_type, msg_len = struct.unpack('!BBHI', data[:8])
                if msg_type == 0x0007:  # Alive Check Request
                    print(f"📥 Received Alive Check Request!\n")
                    return True
                else:
                    print(f"📥 Received message type: 0x{msg_type:04x}\n")
        except socket.timeout:
            print(f"⏰ Timeout - no Alive Check received\n")
            return False
        finally:
            self.sock.settimeout(None)
        
        return False
    
    def close(self):
        """Close connection"""
        if self.sock:
            self.sock.close()
        print("🔌 Connection closed")


def main():
    print("=" * 60)
    print("  VMG Simulator Tester")
    print("  Simple DoIP Client")
    print("=" * 60)
    print()
    
    # Create client
    client = SimpleDoIPClient('127.0.0.1', 13400)
    
    try:
        # Step 1: Connect
        client.connect()
        
        # Step 2: Send Routing Activation
        if not client.send_routing_activation():
            print("❌ Routing Activation failed, exiting...")
            return
        
        # Step 3: Wait for Alive Check
        if client.wait_for_alive_check(timeout=10):
            # Step 4: Respond to Alive Check
            client.send_alive_check_response()
        
        # Step 5: Send Zone Status Report
        client.send_zone_status_report()
        
        # Step 6: Wait a bit for VMG to process
        print("⏳ Waiting 3 seconds...")
        time.sleep(3)
        
        print("\n✅ Test completed successfully!")
        
    except ConnectionRefusedError:
        print("❌ Connection refused - is VMG Simulator running?")
        print("   Run: python vmg_simulator.py")
    except Exception as e:
        print(f"❌ Error: {e}")
    finally:
        client.close()


if __name__ == "__main__":
    main()

