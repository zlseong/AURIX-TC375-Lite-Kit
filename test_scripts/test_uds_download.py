#!/usr/bin/env python3
"""
UDS Download Test Script - Simulates VMG sending software package
Tests 0x34 RequestDownload, 0x36 TransferData, 0x37 RequestTransferExit
"""

import socket
import struct
import time
import random

# DoIP Configuration
VMG_ADDRESS = 0x0E00      # Vehicle Management Gateway
ZGW_ADDRESS = 0x0100      # Zonal Gateway (ECU_091)
TARGET_IP = "192.168.1.10"
TARGET_PORT = 13400

# DoIP Protocol
DOIP_PROTOCOL_VERSION = 0x02
DOIP_INVERSE_VERSION = 0xFD
DOIP_TYPE_ROUTING_ACTIVATION_REQ = 0x0005
DOIP_TYPE_ROUTING_ACTIVATION_RES = 0x0006
DOIP_TYPE_DIAGNOSTIC_MESSAGE = 0x8001
DOIP_TYPE_DIAGNOSTIC_POSITIVE_ACK = 0x8002

# UDS Service IDs
UDS_SID_REQUEST_DOWNLOAD = 0x34
UDS_SID_TRANSFER_DATA = 0x36
UDS_SID_REQUEST_TRANSFER_EXIT = 0x37
UDS_POSITIVE_RESPONSE_OFFSET = 0x40
UDS_SID_NEGATIVE_RESPONSE = 0x7F

# Flash Memory Configuration
FLASH_START_ADDRESS = 0x00100000  # 1MB offset (download area)
TEST_DATA_SIZE = 2048             # 2KB test data


def build_doip_header(payload_type, payload_length):
    """Build DoIP header (8 bytes)"""
    return struct.pack('>BBHL',
                       DOIP_PROTOCOL_VERSION,
                       DOIP_INVERSE_VERSION,
                       payload_type,
                       payload_length)


def build_doip_diagnostic_message(source_addr, target_addr, uds_data):
    """Build DoIP Diagnostic Message"""
    routing = struct.pack('>HH', source_addr, target_addr)
    payload = routing + uds_data
    header = build_doip_header(DOIP_TYPE_DIAGNOSTIC_MESSAGE, len(payload))
    return header + payload


def send_doip_message(sock, message):
    """Send DoIP message and return response"""
    print(f"[TX] Sending {len(message)} bytes...")
    sock.sendall(message)
    
    # Receive response header (8 bytes)
    header = sock.recv(8)
    if len(header) < 8:
        print("[ERROR] Failed to receive DoIP header")
        return None
    
    protocol, inverse, payload_type, payload_len = struct.unpack('>BBHL', header)
    print(f"[RX] DoIP Header: Type=0x{payload_type:04X}, Len={payload_len}")
    
    # Receive payload
    payload = b''
    while len(payload) < payload_len:
        chunk = sock.recv(payload_len - len(payload))
        if not chunk:
            break
        payload += chunk
    
    print(f"[RX] Received {len(payload)} bytes payload")
    return (payload_type, payload)


def uds_request_download(sock, address, size):
    """Send UDS 0x34 RequestDownload"""
    print(f"\n{'='*60}")
    print(f"[UDS 0x34] RequestDownload")
    print(f"  Address: 0x{address:08X}")
    print(f"  Size: {size} bytes ({size // 1024} KB)")
    print(f"{'='*60}")
    
    # Build RequestDownload message
    # Format: [SID] [dataFormatId] [addrLenFormat] [address] [size]
    data_format_id = 0x00  # Compression/Encryption (none)
    addr_len_format = 0x44  # 4 bytes address, 4 bytes size
    
    uds_data = struct.pack('>BBBLL',
                           UDS_SID_REQUEST_DOWNLOAD,
                           data_format_id,
                           addr_len_format,
                           address,
                           size)
    
    message = build_doip_diagnostic_message(VMG_ADDRESS, ZGW_ADDRESS, uds_data)
    
    payload_type, payload = send_doip_message(sock, message)
    
    if payload_type == DOIP_TYPE_DIAGNOSTIC_MESSAGE:
        # Parse UDS response
        source_addr, target_addr = struct.unpack('>HH', payload[:4])
        uds_response = payload[4:]
        
        sid = uds_response[0]
        
        if sid == (UDS_SID_REQUEST_DOWNLOAD + UDS_POSITIVE_RESPONSE_OFFSET):
            print("[SUCCESS] RequestDownload accepted!")
            # Response format: [SID+0x40] [lengthFormatId] [maxBlockLength]
            if len(uds_response) >= 3:
                max_block_len = struct.unpack('>H', uds_response[1:3])[0]
                print(f"  Max block length: {max_block_len} bytes")
                return max_block_len
        elif sid == UDS_SID_NEGATIVE_RESPONSE:
            nrc = uds_response[2]
            print(f"[FAILED] Negative Response Code: 0x{nrc:02X}")
    
    return None


def uds_transfer_data(sock, block_counter, data):
    """Send UDS 0x36 TransferData"""
    # Build TransferData message
    # Format: [SID] [blockCounter] [data...]
    uds_data = struct.pack('>BB', UDS_SID_TRANSFER_DATA, block_counter) + data
    
    message = build_doip_diagnostic_message(VMG_ADDRESS, ZGW_ADDRESS, uds_data)
    
    payload_type, payload = send_doip_message(sock, message)
    
    if payload_type == DOIP_TYPE_DIAGNOSTIC_MESSAGE:
        # Parse UDS response
        uds_response = payload[4:]
        sid = uds_response[0]
        
        if sid == (UDS_SID_TRANSFER_DATA + UDS_POSITIVE_RESPONSE_OFFSET):
            return True
        elif sid == UDS_SID_NEGATIVE_RESPONSE:
            nrc = uds_response[2]
            print(f"[FAILED] Block {block_counter}: NRC=0x{nrc:02X}")
    
    return False


def uds_request_transfer_exit(sock):
    """Send UDS 0x37 RequestTransferExit"""
    print(f"\n{'='*60}")
    print(f"[UDS 0x37] RequestTransferExit")
    print(f"{'='*60}")
    
    # Build RequestTransferExit message
    # Format: [SID]
    uds_data = struct.pack('>B', UDS_SID_REQUEST_TRANSFER_EXIT)
    
    message = build_doip_diagnostic_message(VMG_ADDRESS, ZGW_ADDRESS, uds_data)
    
    payload_type, payload = send_doip_message(sock, message)
    
    if payload_type == DOIP_TYPE_DIAGNOSTIC_MESSAGE:
        # Parse UDS response
        uds_response = payload[4:]
        sid = uds_response[0]
        
        if sid == (UDS_SID_REQUEST_TRANSFER_EXIT + UDS_POSITIVE_RESPONSE_OFFSET):
            print("[SUCCESS] TransferExit complete!")
            return True
        elif sid == UDS_SID_NEGATIVE_RESPONSE:
            nrc = uds_response[2]
            print(f"[FAILED] NRC=0x{nrc:02X}")
    
    return False


def generate_test_data(size):
    """Generate test data pattern"""
    # Pattern: incrementing bytes 0x00, 0x01, ... 0xFF, 0x00, ...
    data = bytearray(size)
    for i in range(size):
        data[i] = i & 0xFF
    return bytes(data)


def main():
    print("="*60)
    print("UDS Download Test - VMG Simulator")
    print("="*60)
    print(f"Target: {TARGET_IP}:{TARGET_PORT}")
    print(f"VMG Address: 0x{VMG_ADDRESS:04X}")
    print(f"ZGW Address: 0x{ZGW_ADDRESS:04X}")
    print()
    
    # Generate test data
    print(f"Generating {TEST_DATA_SIZE} bytes test data...")
    test_data = generate_test_data(TEST_DATA_SIZE)
    print(f"  Pattern: 0x{test_data[0]:02X} 0x{test_data[1]:02X} ... 0x{test_data[-2]:02X} 0x{test_data[-1]:02X}")
    print()
    
    # Connect to ZGW
    print("Connecting to Zonal Gateway...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(10.0)
    
    try:
        sock.connect((TARGET_IP, TARGET_PORT))
        print("[Connected]")
        print()
        
        # Step 1: RequestDownload
        max_block_len = uds_request_download(sock, FLASH_START_ADDRESS, TEST_DATA_SIZE)
        
        if max_block_len is None:
            print("[ERROR] RequestDownload failed!")
            return
        
        time.sleep(0.5)  # Wait for flash erase
        
        # Step 2: TransferData (split into blocks)
        print(f"\n{'='*60}")
        print(f"[UDS 0x36] TransferData")
        print(f"{'='*60}")
        
        block_counter = 1
        offset = 0
        
        while offset < len(test_data):
            # Calculate block size (max_block_len or remaining)
            block_size = min(max_block_len, len(test_data) - offset)
            block_data = test_data[offset:offset + block_size]
            
            print(f"  Block {block_counter}: {block_size} bytes (offset {offset})")
            
            if not uds_transfer_data(sock, block_counter, block_data):
                print(f"[ERROR] TransferData block {block_counter} failed!")
                return
            
            offset += block_size
            block_counter = (block_counter + 1) & 0xFF
            if block_counter == 0:
                block_counter = 1  # Wrap to 1, not 0
            
            time.sleep(0.05)  # Small delay between blocks
        
        print(f"\n  Total: {offset} bytes transferred")
        
        time.sleep(0.2)
        
        # Step 3: RequestTransferExit
        if uds_request_transfer_exit(sock):
            print("\n" + "="*60)
            print("SOFTWARE DOWNLOAD TEST COMPLETE!")
            print("="*60)
            print(f"✓ Downloaded {TEST_DATA_SIZE} bytes to Flash @ 0x{FLASH_START_ADDRESS:08X}")
            print("✓ Data stored in SPI Flash S25FL512S")
            print()
        else:
            print("[ERROR] RequestTransferExit failed!")
    
    except socket.timeout:
        print("[ERROR] Connection timeout!")
    except Exception as e:
        print(f"[ERROR] {e}")
    finally:
        sock.close()
        print("Connection closed.")


if __name__ == '__main__':
    main()

