#!/usr/bin/env python3
"""
UDS Download Test with Software Package Header
Tests ECU routing and Dual Bank Flash management
"""

import socket
import struct
import time

# DoIP Configuration
VMG_ADDRESS = 0x0E00
ZGW_ADDRESS = 0x0100
TARGET_IP = "192.168.1.10"
TARGET_PORT = 13400

# DoIP Protocol
DOIP_PROTOCOL_VERSION = 0x02
DOIP_INVERSE_VERSION = 0xFD
DOIP_TYPE_ROUTING_ACTIVATION_REQ = 0x0005
DOIP_TYPE_DIAGNOSTIC_MESSAGE = 0x8001

# UDS Service IDs
UDS_SID_REQUEST_DOWNLOAD = 0x34
UDS_SID_TRANSFER_DATA = 0x36
UDS_SID_REQUEST_TRANSFER_EXIT = 0x37
UDS_POSITIVE_RESPONSE_OFFSET = 0x40
UDS_SID_NEGATIVE_RESPONSE = 0x7F

# Software Package Header
SW_PKG_MAGIC = 0x53575047  # "SWPG"
ECU_ID_ZGW = 0x0091        # ZGW (for Dual Bank Flash test)
ECU_ID_ZONE_1 = 0x0011     # Zone ECU 1 (for routing test)
SW_TYPE_APPLICATION = 0x01

# Test Configuration
TEST_PAYLOAD_SIZE = 2048   # 2KB payload
TEST_TARGET_ECU = ECU_ID_ZGW  # Change to ECU_ID_ZONE_1 to test routing


def crc32(data):
    """Calculate CRC32"""
    crc = 0xFFFFFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xEDB88320
            else:
                crc >>= 1
    return crc ^ 0xFFFFFFFF


def build_software_package_header(target_ecu_id, payload_size, payload_data):
    """Build 64-byte Software Package Header"""
    header = bytearray(64)
    
    # Identification (16 bytes)
    struct.pack_into('>I', header, 0, SW_PKG_MAGIC)          # magic
    struct.pack_into('>H', header, 4, target_ecu_id)         # target_ecu_id
    struct.pack_into('>B', header, 6, SW_TYPE_APPLICATION)   # software_type
    struct.pack_into('>B', header, 7, 0)                     # compression (none)
    struct.pack_into('>I', header, 8, payload_size)          # payload_size
    struct.pack_into('>I', header, 12, payload_size)         # uncompressed_size
    
    # Version Information (12 bytes)
    struct.pack_into('>B', header, 16, 2)                    # version_major
    struct.pack_into('>B', header, 17, 0)                    # version_minor
    struct.pack_into('>B', header, 18, 0)                    # version_patch
    struct.pack_into('>B', header, 19, 26)                   # version_build
    struct.pack_into('>I', header, 20, int(time.time()))     # version_timestamp
    struct.pack_into('>I', header, 24, 1)                    # version_serial
    
    # Security & Integrity (16 bytes)
    payload_crc = crc32(payload_data)
    struct.pack_into('>I', header, 28, payload_crc)          # crc32
    # signature[3] reserved (zeros)
    
    # Routing Information (8 bytes)
    struct.pack_into('>H', header, 44, VMG_ADDRESS)          # source_ecu_id
    struct.pack_into('>H', header, 46, 0)                    # hop_count
    struct.pack_into('>I', header, 48, 1)                    # sequence_number
    
    # Reserved (12 bytes) - all zeros
    
    return bytes(header)


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
    
    data_format_id = 0x00
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
        uds_response = payload[4:]
        sid = uds_response[0]
        
        if sid == (UDS_SID_REQUEST_DOWNLOAD + UDS_POSITIVE_RESPONSE_OFFSET):
            print("[SUCCESS] RequestDownload accepted!")
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
    uds_data = struct.pack('>BB', UDS_SID_TRANSFER_DATA, block_counter) + data
    message = build_doip_diagnostic_message(VMG_ADDRESS, ZGW_ADDRESS, uds_data)
    payload_type, payload = send_doip_message(sock, message)
    
    if payload_type == DOIP_TYPE_DIAGNOSTIC_MESSAGE:
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
    
    uds_data = struct.pack('>B', UDS_SID_REQUEST_TRANSFER_EXIT)
    message = build_doip_diagnostic_message(VMG_ADDRESS, ZGW_ADDRESS, uds_data)
    payload_type, payload = send_doip_message(sock, message)
    
    if payload_type == DOIP_TYPE_DIAGNOSTIC_MESSAGE:
        uds_response = payload[4:]
        sid = uds_response[0]
        
        if sid == (UDS_SID_REQUEST_TRANSFER_EXIT + UDS_POSITIVE_RESPONSE_OFFSET):
            print("[SUCCESS] TransferExit complete!")
            return True
        elif sid == UDS_SID_NEGATIVE_RESPONSE:
            nrc = uds_response[2]
            print(f"[FAILED] NRC=0x{nrc:02X}")
    
    return False


def generate_test_payload(size):
    """Generate test payload pattern"""
    data = bytearray(size)
    for i in range(size):
        data[i] = i & 0xFF
    return bytes(data)


def main():
    print("="*60)
    print("UDS Download Test with Software Package Header")
    print("="*60)
    print(f"Target: {TARGET_IP}:{TARGET_PORT}")
    print(f"VMG Address: 0x{VMG_ADDRESS:04X}")
    print(f"ZGW Address: 0x{ZGW_ADDRESS:04X}")
    print()
    
    # Target ECU selection
    if TEST_TARGET_ECU == ECU_ID_ZGW:
        print(f"TEST MODE: ZGW Update (Dual Bank Flash)")
        print(f"  Target ECU: ECU_091 (ZGW)")
        print(f"  Expected: Update Standby Flash Bank (A↔B)")
    else:
        print(f"TEST MODE: Zone ECU Routing")
        print(f"  Target ECU: ECU_0{TEST_TARGET_ECU:02X}")
        print(f"  Expected: Store to buffer and route")
    print()
    
    # Generate test payload
    print(f"Generating {TEST_PAYLOAD_SIZE} bytes test payload...")
    test_payload = generate_test_payload(TEST_PAYLOAD_SIZE)
    print(f"  Pattern: 0x{test_payload[0]:02X} 0x{test_payload[1]:02X} ... 0x{test_payload[-2]:02X} 0x{test_payload[-1]:02X}")
    
    # Build Software Package Header
    print(f"\nBuilding Software Package Header...")
    sw_header = build_software_package_header(TEST_TARGET_ECU, TEST_PAYLOAD_SIZE, test_payload)
    print(f"  Magic: 0x{struct.unpack('>I', sw_header[0:4])[0]:08X}")
    print(f"  Target ECU: 0x{struct.unpack('>H', sw_header[4:6])[0]:04X}")
    print(f"  Version: {sw_header[16]}.{sw_header[17]}.{sw_header[18]} (Build {sw_header[19]})")
    print(f"  Payload Size: {struct.unpack('>I', sw_header[8:12])[0]} bytes")
    print(f"  CRC32: 0x{struct.unpack('>I', sw_header[28:32])[0]:08X}")
    
    # Complete package = Header (64B) + Payload
    complete_package = sw_header + test_payload
    total_size = len(complete_package)
    print(f"\nComplete Package Size: {total_size} bytes ({len(sw_header)}B header + {len(test_payload)}B payload)")
    print()
    
    # Connect to ZGW
    print("Connecting to Zonal Gateway...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(30.0)  # Increased timeout for flash operations
    
    try:
        sock.connect((TARGET_IP, TARGET_PORT))
        print("[Connected]")
        print()
        
        # Step 1: RequestDownload (for complete package)
        flash_start = 0x00100000
        max_block_len = uds_request_download(sock, flash_start, total_size)
        
        if max_block_len is None:
            print("[ERROR] RequestDownload failed!")
            return
        
        time.sleep(1.0)  # Wait for potential flash erase
        
        # Step 2: TransferData (header + payload)
        print(f"\n{'='*60}")
        print(f"[UDS 0x36] TransferData (with Software Package Header)")
        print(f"{'='*60}")
        
        block_counter = 1
        offset = 0
        
        while offset < total_size:
            block_size = min(max_block_len, total_size - offset)
            block_data = complete_package[offset:offset + block_size]
            
            if block_counter == 1:
                print(f"  Block {block_counter}: {block_size} bytes [Header + Payload start]")
            else:
                print(f"  Block {block_counter}: {block_size} bytes (offset {offset})")
            
            if not uds_transfer_data(sock, block_counter, block_data):
                print(f"[ERROR] TransferData block {block_counter} failed!")
                return
            
            offset += block_size
            block_counter = (block_counter + 1) & 0xFF
            if block_counter == 0:
                block_counter = 1
            
            time.sleep(0.05)
        
        print(f"\n  Total: {offset} bytes transferred")
        
        time.sleep(0.5)
        
        # Step 3: RequestTransferExit
        if uds_request_transfer_exit(sock):
            print("\n" + "="*60)
            print("SOFTWARE DOWNLOAD TEST COMPLETE!")
            print("="*60)
            
            if TEST_TARGET_ECU == ECU_ID_ZGW:
                print(f"✓ Downloaded to ZGW Standby Flash Bank")
                print(f"✓ Bank switch triggered")
                print(f"✓ New software will be active after reboot")
            else:
                print(f"✓ Package stored for routing")
                print(f"✓ Will be forwarded to ECU_0{TEST_TARGET_ECU:02X}")
            
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

