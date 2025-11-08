#!/usr/bin/env python3
"""
VCI Collection Trigger
Sends command to VMG Simulator to request VCI from Zonal Gateway
"""

import socket
import struct
import sys
import time
from datetime import datetime

# ============================================================================
# DoIP Message Builder
# ============================================================================

class DoIPMessage:
    """DoIP Protocol Message Handler"""
    
    # Protocol Version
    PROTOCOL_VERSION = 0x02
    INVERSE_VERSION = 0xFD
    
    # Payload Types
    DIAGNOSTIC_MESSAGE = 0x8001
    
    def __init__(self, payload_type, payload_data=b''):
        self.protocol_version = self.PROTOCOL_VERSION
        self.inverse_protocol_version = self.INVERSE_VERSION
        self.payload_type = payload_type
        self.payload_data = payload_data
        self.payload_length = len(payload_data)
    
    def to_bytes(self):
        """Serialize DoIP message to bytes"""
        header = struct.pack(
            '!BBHI',  # Network byte order (big-endian)
            self.protocol_version,
            self.inverse_protocol_version,
            self.payload_type,
            self.payload_length
        )
        return header + self.payload_data


# ============================================================================
# UDS Message Builder
# ============================================================================

def build_uds_read_data_request(source_addr, target_addr, did):
    """
    Build UDS 0x22 ReadDataByIdentifier request
    
    Args:
        source_addr: Source logical address (e.g., 0x0200 for VMG)
        target_addr: Target logical address (e.g., 0x0100 for ZGW)
        did: Data Identifier (e.g., 0xF195 for Consolidated VCI)
    
    Returns:
        DoIP Diagnostic Message as bytes
    """
    # DoIP Diagnostic Message Payload:
    # - Source Address (2 bytes)
    # - Target Address (2 bytes)
    # - UDS Data (Service ID + DID)
    
    # UDS 0x22 ReadDataByIdentifier:
    # - Service ID: 0x22
    # - DID: 2 bytes (big-endian)
    
    uds_data = struct.pack('!BH', 0x22, did)  # Service 0x22 + DID
    payload = struct.pack('!HH', source_addr, target_addr) + uds_data
    
    msg = DoIPMessage(DoIPMessage.DIAGNOSTIC_MESSAGE, payload)
    return msg.to_bytes()


# ============================================================================
# Trigger Client
# ============================================================================

class VCITriggerClient:
    """Client to trigger VCI collection from VMG Simulator"""
    
    def __init__(self, vmg_host='127.0.0.1', vmg_port=13401):
        """
        Initialize VCI Trigger Client
        
        Args:
            vmg_host: VMG Simulator host (default: localhost)
            vmg_port: VMG Simulator control port (default: 13401)
        """
        self.vmg_host = vmg_host
        self.vmg_port = vmg_port
        self.sock = None
    
    def log(self, message, level="INFO"):
        """Print timestamped log message"""
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        prefix = {
            "INFO": "📡",
            "SUCCESS": "✅",
            "WARNING": "⚠️",
            "ERROR": "❌",
        }.get(level, "ℹ️")
        print(f"[{timestamp}] {prefix} {message}")
    
    def connect(self):
        """Connect to VMG Simulator control port"""
        try:
            self.log(f"Connecting to VMG Simulator at {self.vmg_host}:{self.vmg_port}...")
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.settimeout(5.0)
            self.sock.connect((self.vmg_host, self.vmg_port))
            self.log("Connected to VMG Simulator", "SUCCESS")
            return True
        except Exception as e:
            self.log(f"Connection failed: {e}", "ERROR")
            return False
    
    def send_command(self, command):
        """
        Send command to VMG Simulator
        
        Args:
            command: Command string (e.g., "REQUEST_VCI", "REQUEST_HEALTH")
        """
        try:
            self.log(f"Sending command: {command}")
            self.sock.send(command.encode('utf-8') + b'\n')
            
            # Wait for response
            response = self.sock.recv(1024).decode('utf-8').strip()
            self.log(f"VMG Response: {response}", "SUCCESS")
            return response
        except Exception as e:
            self.log(f"Command failed: {e}", "ERROR")
            return None
    
    def close(self):
        """Close connection"""
        if self.sock:
            try:
                self.sock.close()
            except:
                pass
            self.log("Connection closed")


# ============================================================================
# Main Entry Point
# ============================================================================

def main():
    print("=" * 60)
    print("  VCI Collection Trigger")
    print("  Send command to VMG Simulator")
    print("=" * 60)
    print()
    
    # Parse command line arguments
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python trigger_vci_collection.py <command> [vmg_host] [vmg_port]")
        print()
        print("Commands:")
        print("  vci     - Request Consolidated VCI (DID 0xF195)")
        print("  health  - Request Health Status (DID 0xF1A0)")
        print()
        print("Examples:")
        print("  python trigger_vci_collection.py vci")
        print("  python trigger_vci_collection.py health")
        print("  python trigger_vci_collection.py vci 192.168.1.100 13401")
        sys.exit(1)
    
    command = sys.argv[1].lower()
    vmg_host = sys.argv[2] if len(sys.argv) > 2 else '127.0.0.1'
    vmg_port = int(sys.argv[3]) if len(sys.argv) > 3 else 13401
    
    # Map command to VMG command string
    command_map = {
        'vci': 'REQUEST_VCI',
        'health': 'REQUEST_HEALTH',
    }
    
    if command not in command_map:
        print(f"❌ Unknown command: {command}")
        print(f"   Valid commands: {', '.join(command_map.keys())}")
        sys.exit(1)
    
    vmg_command = command_map[command]
    
    # Create trigger client
    client = VCITriggerClient(vmg_host, vmg_port)
    
    try:
        # Connect to VMG
        if not client.connect():
            sys.exit(1)
        
        # Send command
        time.sleep(0.5)  # Brief delay
        response = client.send_command(vmg_command)
        
        if response:
            print()
            print("✅ Command sent successfully!")
            print(f"   Watch the VMG Simulator console for UDS request")
        else:
            print()
            print("❌ Command failed")
            sys.exit(1)
    
    except KeyboardInterrupt:
        print("\n⚠️ Interrupted by user")
    except Exception as e:
        print(f"\n❌ Error: {e}")
        sys.exit(1)
    finally:
        client.close()


if __name__ == "__main__":
    main()

