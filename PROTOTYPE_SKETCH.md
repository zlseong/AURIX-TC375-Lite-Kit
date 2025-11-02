# Zonal Gateway - Prototype Sketch

**Platform**: TC375 Lite Kit (Ethernet PHY)  
**Version**: 1.0  
**Date**: 2025-11-01  
**Status**: Design Phase

---

## 🌐 전체 시스템 아키텍처

### 4-Tier 계층 구조

```
┌─────────────────────────────────────────────┐
│              Cloud Server                   │
│         (OTA Management)                    │
└──────────────────┬──────────────────────────┘
                   │ HTTPS/MQTT (PQC-TLS)
                   │ JSON Messages
        ┌──────────▼──────────┐
        │       VMG (CCU)     │  ← MacBook Air (Linux x86)
        │  Central Gateway    │     192.168.1.1
        │  - DoIP Server      │
        │  - PQC-TLS Client   │
        │  - VCI Aggregation  │
        └──────────┬──────────┘
                   │ DoIP (TCP 13400)
                   │ Plain (No PQC)
        ┌──────────┼──────────┐
        │          │          │
┌───────▼──────┐ ┌─▼────────┐ ┌─▼────────┐
│ Zonal GW #1  │ │ ZG #2    │ │ ZG #3    │  ← TC375 (이 프로젝트!)
│ Zone 1       │ │ Zone 2   │ │ Zone 3   │     192.168.1.10/20/30
│ .10          │ │ .20      │ │ .30      │
│              │ │          │ │          │
│ Server+Client│ │          │ │          │
└───────┬──────┘ └────┬─────┘ └────┬─────┘
        │ DoIP         │            │
        │ (TCP 13400)  │            │
    ┌───┼───┐          │            │
    │   │   │          │            │
┌───▼─┐ │ ┌─▼──┐   ┌──▼──┐     ┌──▼──┐
│ECU 1│ │ │ECU2│   │ECU 4│     │ECU 6│  ← TC375 (별도 디바이스)
│.100 │ │ │.101│   │.110 │     │.120 │
└─────┘ │ └────┘   └─────┘     └─────┘
        │
    ┌───▼──┐
    │ECU 3 │
    │.102  │
    └──────┘
```

### Zonal Gateway의 위치
```
이 프로젝트 = Zonal Gateway #1 (Zone 1)
- IP: 192.168.1.10
- Ethernet PHY를 통해 VMG 및 Zone 내 ECU들과 연결
- 자신도 일종의 ECU (Self-Update 가능)
```

---

## 🎯 Zonal Gateway의 이중 역할

### 1. Upstream (VMG와 통신) - **Client 역할**

```
Zonal GW (192.168.1.10) → VMG (192.168.1.1)
                           DoIP Client
```

**역할:**
- ✅ VMG에 연결 (TCP 13400)
- ✅ Routing Activation
- ✅ **Heartbeat 취합 전송** (10초마다)
  - Zone 내 ECU들의 Heartbeat 수집
  - 자신의 Heartbeat 포함
  - VMG로 통합 전송
- ✅ **Zone VCI 전송** (60초마다)
  - Zone 내 ECU VCI 수집
  - 자신의 VCI 포함
  - VMG로 통합 전송
- ✅ **Zone 상태 전송**
- ✅ **UDS 요청 처리** (VMG로부터)
  - VMG의 진단 요청 수신
  - Zone 내 ECU로 라우팅 또는 자신이 직접 처리

### 2. Downstream (ECU들과 통신) - **Server 역할**

```
ECU 1,2,3 (192.168.1.100~102) → Zonal GW (192.168.1.10)
                                 DoIP Server
```

**역할:**
- ✅ ECU들의 연결 수락 (TCP 13400)
- ✅ Vehicle Discovery 응답 (UDP 13400)
- ✅ **UDS 요청 처리** (ECU들로부터)
- ✅ **Heartbeat 수집** (각 ECU로부터 10초마다)
- ✅ Zone 내 ECU 관리
- ✅ **OTA 배포** (zone.bin 수신 → ECU별 분배)
- ✅ **Self-Update** (zone.bin에서 자신의 펌웨어 추출)

---

## 🔄 주요 메시지 흐름 시퀀스

### 시나리오 1: 시동 & Discovery

```
Server         VMG            Zonal GW        ECU #1
  │             │                │              │
  │             │                │◄─────────────┤ Power On
  │             │                │              │
  │             │                │◄─UDP Bcast───┤ "ZG 찾기"
  │             │                │              │ (255.255.255.255:13400)
  │             │                │              │
  │             │                ├──UDP Reply──►│ "나는 ZG #1"
  │             │                │              │ VIN: KMHGH...
  │             │                │              │ Address: 0x0201
  │             │                │              │
  │             │                │◄──TCP Conn───┤ DoIP Connect
  │             │                │              │ 192.168.1.10:13400
  │             │                │              │
  │             │                │◄──Routing────┤ Activation Request
  │             │                ├──Routing────►│ Activation Response
  │             │                │              │ (Success: 0x10)
  │             │                │              │
  │             │◄───TCP Conn────┤              │ ZG → VMG
  │             │                │              │ 192.168.1.1:13400
  │             │                │              │
  │             │◄───Routing─────┤              │ Activation Request
  │             ├───Routing─────►│              │ Activation Response
  │             │                │              │
  │◄────MQTT────┤                │              │ VMG Wake Up
  │────ACK─────►│                │              │
```

### 시나리오 2: VCI 수집 (ECU → ZG → VMG → Server)

```
Server         VMG            Zonal GW        ECU #1
  │             │                │              │
  │──Request───►│                │              │ "VCI 수집해" (MQTT)
  │    VCI      │                │              │
  │             │──Collect VCI──►│              │ "Zone 1 VCI" (DoIP UDS)
  │             │                │              │
  │             │                │──UDS 0x22───►│ Read VIN (F190)
  │             │                │◄─VIN─────────┤ KMHGH4JH1NU123456
  │             │                │              │
  │             │                │──UDS 0x22───►│ Read SW Ver (F195)
  │             │                │◄─SW v1.0.0───┤ "v1.0.0"
  │             │                │              │
  │             │                │──UDS 0x22───►│ Read HW Ver (F193)
  │             │                │◄─HW Ver──────┤ "TC375TP-v2.0"
  │             │                │              │
  │             │                │ [자신의 VCI]  │
  │             │                │ 추가          │
  │             │                │              │
  │             │                │ [Zone VCI]   │
  │             │                │ 집계:        │
  │             │                │ - ECU 1,2,3  │
  │             │                │ - ZG 자신    │
  │             │                │              │
  │             │◄──Zone VCI────┤              │ Zone 1 VCI (DoIP)
  │             │                │              │ {
  │             │                │              │   "zone_id": 1,
  │             │                │              │   "zg": {...},      ← 추가!
  │             │                │              │   "ecus": [
  │             │                │              │     {ecu1}, {ecu2}, {ecu3}
  │             │                │              │   ]
  │             │                │              │ }
  │             │                │              │
  │             │ [Vehicle VCI]  │              │
  │             │ = Zone1+2+3    │              │
  │             │                │              │
  │◄──VCI Report┤                │              │ Vehicle VCI (MQTT)
```

### 시나리오 3: Heartbeat 취합 (ECU → ZG → VMG)

```
VMG            Zonal GW        ECU #1          ECU #2
 │                │              │               │
 │                │◄─Heartbeat───┤               │ UDS 0x3E (10초마다)
 │                │              │               │
 │                │◄─────────────┴──Heartbeat───┤ UDS 0x3E (10초마다)
 │                │                              │
 │                │ [자신의 Heartbeat 추가]       │
 │                │                              │
 │                │ [통합 Heartbeat]              │
 │                │ - ECU #1: Alive             │
 │                │ - ECU #2: Alive             │
 │                │ - ECU #3: Alive             │
 │                │ - ZG 자신: Alive            │
 │                │                              │
 │◄──Heartbeat────┤                              │ 통합 Heartbeat (DoIP)
 │   (통합)       │                              │
```

### 시나리오 4: OTA 업데이트 (4 Phase)

#### **Phase 1: Package Transfer**

```
Server         VMG            Zonal GW        ECU #1
  │             │                │              │
  │──HTTPS─────►│                │              │ Global Package (100 MB)
  │ Download    │                │              │
  │             │                │              │
  │             │ [Extract]      │              │
  │             │ zone1.bin      │              │
  │             │ (20 MB)        │              │
  │             │                │              │
  │             │──TCP Send─────►│              │ Zone 1 Package (20 MB)
  │             │                │              │
  │             │                │ [Extract]    │
  │             │                │ - zg.bin     │ ← ZG 자신의 펌웨어!
  │             │                │   (600 KB)   │
  │             │                │ - ecu1.bin   │
  │             │                │   (5 MB)     │
  │             │                │ - ecu2.bin   │
  │             │                │   (5 MB)     │
  │             │                │ - ecu3.bin   │
  │             │                │   (5 MB)     │
  │             │                │              │
  │             │                │ [Self-Update]│
  │             │                ├─Write B Bank►│ ZG 자신을 Bank B에 쓰기
  │             │                │              │ @ 0x80341000
  │             │                │              │
  │             │                │──UDS 0x34───►│ Request Download
  │             │                │              │ (ECU #1 대상)
  │             │                │              │
  │             │                │──UDS 0x36───►│ Transfer Data
  │             │                │              │ (반복...)
  │             │                │              │
  │             │                │──UDS 0x37───►│ Transfer Exit
  │             │                │◄─Success─────┤
```

#### **Phase 2: Readiness Collection**

```
Server         VMG            Zonal GW        ECU #1
  │             │                │              │
  │──Request───►│                │              │ "준비 확인"
  │  Readiness  │                │              │
  │             │                │              │
  │             │──Check Ready──►│              │
  │             │                │              │
  │             │                │──Check──────►│ Battery? Park?
  │             │                │◄─85% Parked──┤
  │             │                │              │
  │             │                │ [자신도 체크] │
  │             │                │ - Battery: OK│
  │             │                │ - Storage: OK│
  │             │                │              │
  │             │                │ [집계]        │
  │             │                │ - ZG: Ready  │
  │             │                │ - ECU 1: Ready│
  │             │                │ - ECU 2: Ready│
  │             │                │              │
  │             │◄──Zone Ready───┤              │
  │◄──Ready─────┤                │              │ All Zone Ready
```

#### **Phase 3: Activation**

```
Driver         VMG            Zonal GW        ECU #1
  │             │                │              │
  │──Allow─────►│                │              │ "설치 승인"
  │  Install    │                │              │
  │             │                │              │
  │             │──Activation───►│              │
  │             │                │              │
  │             │                │ [자신 설치]   │
  │             │                ├─Set Bank B──►│ ZG 자신
  │             │                │              │
  │             │                │──UDS 0x31───►│ Install (ECU #1)
  │             │                │              │
  │             │                │              ├─Set Bank B─►
  │             │                │              │
  │             │                ├─Reboot──────►│ ZG 재부팅
  │             │                │              │
  │             │                │              │ [ZG Boot from B]
  │             │                │              │ ZG v1.1.0
  │             │                │              │
  │             │                │──Reboot CMD─►│ ECU 재부팅
  │             │                │              │
  │             │                │              │ [ECU Boot from B]
  │             │                │              │ ECU v1.1.0
```

#### **Phase 4: Result Report**

```
Server         VMG            Zonal GW        ECU #1
  │             │                │              │
  │             │                │ [Self Test]  │
  │             │                │ - ZG v1.1.0  │
  │             │                │ - RAM OK     │
  │             │                │ - Network OK │
  │             │                │              │
  │             │                │◄─Result──────┤ ECU Self Test
  │             │                │              │ ECU v1.1.0 OK
  │             │                │              │
  │             │                │ [집계]        │
  │             │                │ - ZG: Success│
  │             │                │ - ECU 1: OK  │
  │             │                │ - ECU 2: OK  │
  │             │                │              │
  │             │◄──Zone Result──┤              │
  │◄──Report────┤                │              │ Zone 1: Success
  │   (MQTT)    │                │              │
```

---

## 📋 프로젝트 개요

### 목표
TC375 Lite Kit에서 동작하는 Zonal Gateway 구현
- **Dual Role**: Zone 내 ECU들의 서버 + VMG의 클라이언트
- **Self-Update**: 자신도 OTA 업데이트 가능 (zone.bin에서 추출)
- **Dual Bank OTA**: Bootloader + Application 모두 OTA 가능
- **Real-time**: FreeRTOS 기반 멀티태스킹
- **프로토콜**: DoIP (ISO 13400) + UDS (ISO 14229)

### 개발 일정
**4일 집중 개발** (하루 8시간)
- Day 1: 하드웨어 + 네트워크 (lwIP)
- Day 2: DoIP + UDS 프로토콜
- Day 3: Dual Bank Bootloader
- Day 4: OTA + 통합 테스트

---

## 🔑 핵심 구현 포인트

### 1. Heartbeat 취합 로직
```c
// Zone 내 모든 ECU의 Heartbeat 수집
typedef struct {
    char ecu_id[32];
    uint32_t last_heartbeat_time;
    bool is_alive;
} ECUHeartbeat_t;

// 10초마다 VMG로 전송
void send_aggregated_heartbeat_to_vmg() {
    // 1. 각 ECU의 Heartbeat 상태 확인
    // 2. 자신의 Heartbeat 추가
    // 3. 통합하여 VMG로 전송
}
```

### 2. Self-Update 로직
```c
// zone.bin 파싱
typedef struct {
    uint32_t zg_firmware_offset;    // ZG 펌웨어 위치
    uint32_t zg_firmware_size;      // ZG 펌웨어 크기
    uint32_t ecu1_firmware_offset;
    uint32_t ecu1_firmware_size;
    // ...
} ZonePackageHeader_t;

// Self-Update 절차
void zg_self_update(uint8_t* zone_bin, size_t size) {
    // 1. zone.bin 헤더 파싱
    ZonePackageHeader_t* header = parse_zone_package(zone_bin);
    
    // 2. 자신의 펌웨어 추출
    uint8_t* zg_firmware = zone_bin + header->zg_firmware_offset;
    
    // 3. Bank B에 쓰기
    flash_erase_bank(BANK_B);
    flash_write_bank(BANK_B, zg_firmware, header->zg_firmware_size);
    
    // 4. 검증
    if (verify_firmware(BANK_B)) {
        set_active_bank(BANK_B);
    }
}
```

### 3. UDS 라우팅 로직
```c
// VMG로부터 UDS 요청 수신 시
void handle_uds_from_vmg(uint16_t target_address, uint8_t* uds_data) {
    if (target_address == MY_ADDRESS) {
        // 나에게 온 요청 → 직접 처리
        handle_uds_locally(uds_data);
    } else {
        // Zone 내 ECU에게 라우팅
        forward_to_ecu(target_address, uds_data);
    }
}
```

---

## 📊 세부 사양

---

## 💾 메모리 맵 (TC375: 6 MB Flash, 512 KB RAM)

### Flash 메모리 구조

```
TC375 PFLASH: 6 MB (6,291,456 bytes = 0x600000)

┌───────────────────────────────────────────────────┐
│ 0x80000000  BMI Header (256 B)            [고정]  │
├───────────────────────────────────────────────────┤
│ 0x80000100  SSW (Stage 1) (64 KB)         [고정]  │  64 KB
│             - 절대 업데이트 안 함                  │
│             - Bootloader A/B 선택                 │
│             - CRC 검증만                          │
├───────────────────────────────────────────────────┤
│ 0x80010000  Reserved (64 KB)              [여유]  │  64 KB
├───────────────────────────────────────────────────┤
│ 0x80020000  Bootloader A (128 KB)    [OTA 가능]   │  128 KB
│             - Meta: 0x80020000 (4 KB)             │
│             - Code: 0x80021000 (124 KB)           │
│             - 실제 사용: ~65 KB (51%)              │
├───────────────────────────────────────────────────┤
│ 0x80040000  Bootloader B (128 KB)    [OTA 가능]   │  128 KB
│             - Meta: 0x80040000 (4 KB)             │
│             - Code: 0x80041000 (124 KB)           │
│             - 실제 사용: ~65 KB (51%)              │
├───────────────────────────────────────────────────┤
│ 0x80060000  Application A (2.88 MB)  [OTA 가능]   │  2.88 MB
│             - Meta: 0x80060000 (4 KB)             │
│             - Code: 0x80061000 (~2.88 MB)         │
│             - 실제 사용: ~600 KB (21%)            │
├───────────────────────────────────────────────────┤
│ 0x80340000  Application B (2.88 MB)  [OTA 가능]   │  2.88 MB
│             - Meta: 0x80340000 (4 KB)             │
│             - Code: 0x80341000 (~2.88 MB)         │
│             - 실제 사용: ~600 KB (21%)            │
├───────────────────────────────────────────────────┤
│ 0x80620000  Reserved (128 KB)            [여유]   │  128 KB
└───────────────────────────────────────────────────┘

총합: 6 MB (딱 맞음!)
```

### DFLASH 메모리 (384 KB)

```
0xAF000000  Boot Config (4 KB)        - Active Bank 정보
0xAF001000  Application Data (60 KB)  - VCI, 설정 등
0xAF010000  OTA Buffer (64 KB)        - 임시 다운로드
0xAF400000  UCB (Reserved)            - Infineon 전용
```

### RAM 사용량 (512 KB)

| 컴포넌트 | 크기 | 설명 |
|---------|------|------|
| FreeRTOS Kernel | 2 KB | Task 관리 |
| Task Stacks (6개) | 30 KB | 각 Task 별 Stack |
| lwIP Heap | 32 KB | TCP 버퍼 |
| lwIP Buffers | 20 KB | Packet buffers |
| DoIP RX/TX Buffer | 16 KB | 통신 버퍼 |
| Flash Write Buffer | 16 KB | OTA 다운로드 |
| Application Data | 30 KB | Zone VCI, 상태 |
| BSS + Data | 20 KB | 전역 변수 |
| 예비 | 20 KB | 동적 할당 |
| **총합** | **~190 KB** | **37% 사용** |

---

## 🏗️ 3-Tier Boot Architecture

```
Power On / Reset
     │
     ▼
┌──────────────────────────────────┐
│  Stage 1: SSW                    │  64 KB
│  @ 0x80000100                    │
│  ────────────────────────────    │
│  - ROM-like (절대 불변)           │
│  - Bootloader A/B 선택            │
│  - CRC 검증                       │
│  - Jump to Bootloader            │
└────────────┬─────────────────────┘
             │ 점프
      ┌──────┴──────┐
      ▼              ▼
┌──────────┐    ┌──────────┐
│ Stage 2A │    │ Stage 2B │        128 KB each
│ Boot A   │    │ Boot B   │        @ 0x80021000 / 0x80041000
│ (Active) │    │ (Backup) │
│          │    │          │        ← OTA 업데이트 가능! ✅
│          │    │          │
│ ──────── │    │ ──────── │
│ - 하드웨어│    │ - 하드웨어│
│   초기화  │    │   초기화  │
│ - App 검증│    │ - App 검증│
│   (CRC32)│    │   (CRC32)│
│ - Boot   │    │ - Boot   │
│   Count  │    │   Count  │
│ - Jump   │    │ - Jump   │
└─────┬────┘    └─────┬────┘
      │ 점프          │ 점프
      ▼              ▼
┌──────────┐    ┌──────────┐
│  App A   │    │  App B   │        2.88 MB each
│ (Active) │    │ (Backup) │        @ 0x80061000 / 0x80341000
│          │    │          │
│          │    │          │        ← OTA 업데이트 가능! ✅
└──────────┘    └──────────┘
```

### Fail-Safe 메커니즘 (3단계 방어)

```
Level 1: SSW (Stage 1)
─────────────────────
Boot A CRC 실패
  ↓
SSW → Boot B (Fallback)
  ✅ 복구 성공!

Level 2: Bootloader (Stage 2)
──────────────────────────
Boot A: App A 3회 부팅 실패
  ↓
Boot A → App B (Fallback)
  ✅ 복구 성공!

Level 3: 최악의 경우
─────────────────
Boot A/B 둘 다 CRC 실패
  ↓
USB DFU Mode 진입
  ⚠️ 수동 복구 필요
```

---

## 🔧 소프트웨어 아키텍처

### FreeRTOS Task 구조

```c
┌─────────────────────────────────────────────┐
│                 FreeRTOS                    │
│              (Priority-based)               │
└──┬──────┬──────┬──────┬──────┬─────────────┘
   │      │      │      │      │
   │      │      │      │      │
┌──▼──┐ ┌─▼──┐ ┌─▼──┐ ┌─▼──┐ ┌▼────┐ ┌─────┐
│Net  │ │DoIP│ │DoIP│ │VCI │ │OTA  │ │Watch│
│RX   │ │Srv │ │Cli │ │Coll│ │Mgr  │ │dog  │
│     │ │    │ │    │ │    │ │     │ │     │
│High │ │High│ │High│ │Med │ │Low  │ │High │
│Pri  │ │Pri │ │Pri │ │Pri │ │Pri  │ │Pri  │
└─────┘ └────┘ └────┘ └────┘ └─────┘ └─────┘
  1ms    Block  10s    60s     10ms   100ms
```

### Task 상세

| Task | 우선순위 | Stack | 주기 | 역할 |
|------|---------|-------|------|------|
| **Network RX** | High | 1 KB | 1ms | Ethernet 패킷 수신 |
| **DoIP Server** | High | 4 KB | Block | ECU 요청 처리 (Downstream) |
| **DoIP Client** | High | 2 KB | 10s | VMG Heartbeat 취합 전송 (Upstream) |
| **VCI Collection** | Normal | 3 KB | 60s | Zone VCI 수집 (ZG+ECU) |
| **OTA Manager** | Low | 2 KB | 10ms | Self-Update + ECU OTA 배포 |
| **Watchdog** | High | 512B | 100ms | 시스템 모니터링 |

---

## 📡 네트워크 구성

### IP 주소 할당
```
VMG (CCU):        192.168.1.1
Zone 1 Gateway:   192.168.1.10   ← 이 프로젝트
Zone 2 Gateway:   192.168.1.20
Zone 3 Gateway:   192.168.1.30
Zone ECUs:        192.168.1.100+
```

### 포트 할당
```
DoIP (TCP):       13400   - 진단 통신
DoIP (UDP):       13400   - Vehicle Discovery
JSON (TCP):       8765    - 제어 메시지
```

### DoIP 통신 구조

```
┌─────────────────────────────────────────────┐
│               VMG (CCU)                     │
│            192.168.1.1:13400                │
└──────────────────┬──────────────────────────┘
                   │ DoIP (TCP)
                   │ Client → Server
        ┌──────────┼──────────┐
        │          │          │
┌───────▼──────┐ ┌─▼────────┐ ┌▼──────────┐
│ Zone 1 GW    │ │ Zone 2 GW│ │ Zone 3 GW │
│ :13400       │ │ :13400   │ │ :13400    │
│              │ │          │ │           │
│ Server/Client│ │          │ │           │
└───┬──────┬───┘ └──────────┘ └───────────┘
    │      │ DoIP (TCP)
    │      │ Client → Server
┌───▼──┐ ┌─▼────┐
│ECU #1│ │ECU #2│ (Zone 1)
└──────┘ └──────┘
```

---

## 🔄 DoIP/UDS 프로토콜

### DoIP Generic Header (ISO 13400)
```c
struct DoIPHeader {
    uint8_t  protocol_version;     // 0x02
    uint8_t  inverse_version;      // 0xFD
    uint16_t payload_type;         // Big-endian
    uint32_t payload_length;       // Big-endian
    uint8_t  payload[];            // 가변 길이
};
```

### 주요 Payload Types
```c
0x0001  Vehicle Identification Request  (UDP)
0x0004  Vehicle Identification Response (UDP)
0x0005  Routing Activation Request      (TCP)
0x0006  Routing Activation Response     (TCP)
0x0007  Alive Check Request
0x0008  Alive Check Response
0x8001  Diagnostic Message              (UDS)
0x8002  Diagnostic Positive ACK
0x8003  Diagnostic Negative ACK
```

### 구현 필수 UDS 서비스 (ISO 14229)

| SID | 서비스 | 용도 | 우선순위 |
|-----|--------|------|---------|
| **0x10** | Diagnostic Session Control | 세션 전환 | 필수 |
| **0x22** | Read Data By Identifier | VCI 읽기 | 필수 |
| **0x3E** | Tester Present | Heartbeat | 필수 |
| **0x34** | Request Download | OTA 시작 | 필수 |
| **0x36** | Transfer Data | OTA 데이터 | 필수 |
| **0x37** | Request Transfer Exit | OTA 완료 | 필수 |
| 0x11 | ECU Reset | 재부팅 | 선택 |
| 0x27 | Security Access | 보안 | 선택 |
| 0x31 | Routine Control | 설치 명령 | 선택 |

### 주요 Data Identifier (DID)
```c
0xF186  Active Diagnostic Session
0xF187  Vehicle Manufacturer Spare Part Number
0xF18A  System Supplier Identifier
0xF18C  ECU Serial Number
0xF190  VIN (Vehicle Identification Number)
0xF191  ECU Hardware Number
0xF193  Hardware Version Number
0xF194  System Supplier Specific
0xF195  Software Version Number
0xF197  System Name or Engine Type
0xF198  Repair Shop Code
0xF199  Programming Date
```

---

## 🚀 OTA 업데이트 플로우 (세부)

### Phase 1: Package Transfer
```
VMG                ZG                 ECU
 │                 │                  │
 │─zone1.bin──────►│                  │  Zone Package (20 MB)
 │                 │                  │  from VMG
 │                 │                  │
 │                 │ [Extract]        │
 │                 │ - zg.bin (600KB) │  ← ZG 자신!
 │                 │ - ecu1.bin (5MB) │
 │                 │ - ecu2.bin (5MB) │
 │                 │                  │
 │                 │ [Self-Update]    │
 │                 ├─Write Bank B────►│  ZG 자신을 Bank B에 쓰기
 │                 │ @ 0x80341000     │  (600 KB)
 │                 │                  │
 │                 │─UDS 0x34────────►│  Request Download (ECU #1)
 │                 │ (Target: App B)  │  @ 0x80341000
 │                 │                  │
 │                 │◄───ACK───────────┤
 │                 │                  │
 │                 │─UDS 0x36────────►│  Transfer Data
 │                 │ Block #1 (4KB)   │  (반복)
 │                 │                  ├─Flash Write──►
 │                 │◄───ACK───────────┤
 │                 │                  │
 │                 │─UDS 0x36────────►│  Transfer Data
 │                 │ Block #2         │  (계속...)
 │                 │                  │
 │                 │      ...         │
 │                 │                  │
 │                 │─UDS 0x37────────►│  Request Transfer Exit
 │                 │                  ├─CRC Check────►
 │                 │◄───Success───────┤
```

### Phase 2: Readiness Collection
```
VMG                ZG                 ECU
 │                 │                  │
 │─Check Ready────►│                  │  준비 확인
 │                 │                  │
 │                 │ [자신 체크]       │
 │                 │ - Battery: OK    │
 │                 │ - Storage: OK    │
 │                 │ - Bank B: Valid  │
 │                 │                  │
 │                 │─Check Ready─────►│  ECU 준비 확인
 │                 │                  │
 │                 │◄─Battery 85%────┤
 │                 │◄─Parked─────────┤
 │                 │◄─Bank B Valid───┤
 │                 │                  │
 │                 │ [집계]            │
 │                 │ - ZG: Ready      │
 │                 │ - ECU #1: Ready  │
 │                 │ - ECU #2: Ready  │
 │                 │                  │
 │◄─Zone Ready─────┤                  │  All Ready
```

### Phase 3: Activation (Atomic Switch)
```
VMG                ZG                 ECU
 │                 │                  │
 │─Activation─────►│                  │  설치 명령
 │                 │                  │
 │                 │ [자신 설치]       │
 │                 ├─Set Bank B──────►│  ZG: Bank B 활성화
 │                 │                  │
 │                 │─UDS 0x31────────►│  Install Command (ECU)
 │                 │                  │
 │                 │                  ├─Set Bank B───►
 │                 │                  │
 │                 ├─Reboot──────────►│  ZG 재부팅
 │                 │                  │
 │                 │                  │  [ZG boots from Bank B]
 │                 │                  │  ZG v1.1.0
 │                 │                  │
 │                 │─Reboot CMD──────►│  ECU 재부팅 명령
 │                 │                  │
 │                 │                  │  [ECU boots from Bank B]
 │                 │                  │  ECU v1.1.0

Boot Sequence:
 ZG: SSW → Boot A → App B (v1.1.0) ✅
 ECU: SSW → Boot A → App B (v1.1.0) ✅
```

### Phase 4: Result Report
```
VMG                ZG                 ECU
 │                 │                  │
 │                 │ [Self Test]      │
 │                 │ - ZG v1.1.0      │
 │                 │ - RAM OK         │
 │                 │ - Network OK     │
 │                 │ - VMG Connect OK │
 │                 │                  │
 │                 │                  │  [ECU Self Test]
 │                 │◄─Result──────────┤  - ECU v1.1.0
 │                 │                  │  - HW OK
 │                 │                  │
 │                 │ [집계]            │
 │                 │ - ZG: Success    │
 │                 │ - ECU #1: Success│
 │                 │ - ECU #2: Success│
 │                 │                  │
 │◄─Zone Result────┤                  │  Zone 1: All Success
 │                 │                  │
 
성공 시:
 1. Mark FW_VALID (ZG + ECU)
 2. Reset Boot Count
 3. 정상 동작
 
실패 시 (Boot Count >= 3):
 1. Bootloader가 자동 Rollback
 2. Bank A로 복귀 (이전 버전)
 3. 에러 리포트 전송
```

---

## 📊 lwIP 최적화 설정

### lwipopts.h (메모리 최소화)
```c
/* 연결 수 제한 */
#define MEMP_NUM_TCP_PCB              10    // TCP 연결 (VMG 1 + ECU 8)
#define MEMP_NUM_TCP_PCB_LISTEN       2     // Listen 소켓

/* 버퍼 크기 */
#define TCP_MSS                       1460  // MTU 1500 - 40
#define TCP_SND_BUF                   (4 * TCP_MSS)  // 5.8 KB
#define TCP_WND                       (4 * TCP_MSS)  // 5.8 KB

/* 패킷 버퍼 */
#define PBUF_POOL_SIZE                16    // 16개
#define PBUF_POOL_BUFSIZE             512   // 512 bytes

/* Heap */
#define MEM_SIZE                      (32*1024)  // 32 KB

/* 불필요한 기능 제거 */
#define LWIP_DHCP                     0     // Static IP 사용
#define LWIP_DNS                      0     // DNS 불필요
#define LWIP_IGMP                     0     // Multicast 불필요
#define LWIP_STATS                    0     // 통계 불필요
#define LWIP_NETIF_LOOPBACK           0     // Loopback 불필요

/* 결과 */
// Flash: ~80 KB (최적화)
// RAM:   ~52 KB (32KB Heap + 20KB buffers)
```

---

## 🔐 Boot Configuration (EEPROM)

### 구조체 정의
```c
typedef struct {
    // Bootloader 선택
    uint8_t bootloader_active;           // 0=A, 1=B
    uint8_t bootloader_boot_count_a;     // 부팅 시도 횟수
    uint8_t bootloader_boot_count_b;
    
    // Application 선택
    uint8_t application_active;          // 0=A, 1=B
    uint8_t application_boot_count_a;
    uint8_t application_boot_count_b;
    
    // CRC
    uint32_t crc;                        // 구조체 무결성
} __attribute__((packed)) BootConfig;

typedef struct {
    uint32_t magic_number;               // 0xA5A5A5A5
    uint32_t firmware_version;
    uint32_t firmware_size;
    uint32_t crc32;
    uint8_t  signature[64];              // 서명 (선택적)
    uint32_t build_timestamp;
    uint32_t boot_count;
    uint8_t  status;                     // 0=Invalid, 1=Valid, 2=Testing
    uint8_t  reserved[183];
} __attribute__((packed)) BankMetadata;  // 256 bytes
```

---

## 🎯 4일 개발 계획

### Day 1: 하드웨어 + 네트워크 (lwIP)
**오전 (4시간)**
- [ ] TC375 프로젝트 생성 (AURIX Studio)
- [ ] CPU/Clock/Interrupt 초기화
- [ ] Ethernet PHY 초기화 (GETH 모듈)
- [ ] lwIP 통합 (기존 예제 활용)

**오후 (4시간)**
- [ ] Static IP 설정 (192.168.1.10)
- [ ] Ping 테스트
- [ ] TCP Socket 기본 테스트
- [ ] UART 디버그 출력

**목표**: 네트워크 통신 ✅

---

### Day 2: DoIP + UDS 프로토콜
**오전 (4시간)**
- [ ] DoIP Server (TCP 13400)
  - Generic Header 파싱
  - Routing Activation
- [ ] DoIP Client (VMG 연결)
  - TCP Connect
  - Routing Activation

**오후 (4시간)**
- [ ] UDS Handler 구현
  - 0x10 (Diagnostic Session)
  - 0x22 (Read DID: VIN, SW Ver)
  - 0x3E (Tester Present)
- [ ] Zone VCI 수집 테스트
- [ ] VMG 연동 테스트

**목표**: DoIP/UDS 통신 ✅

---

### Day 3: Dual Bank Bootloader
**오전 (4시간)**
- [ ] 메모리 맵 정의 (boot_common.h)
- [ ] Linker Scripts 작성
  - SSW Linker (0x80000100)
  - Bootloader A/B Linker
  - Application A/B Linker
- [ ] SSW (Stage 1) 구현
  - Boot Config 읽기
  - Bootloader 선택
  - Jump to Bootloader

**오후 (4시간)**
- [ ] Stage 2 Bootloader
  - Application 검증 (CRC32)
  - Boot Count 체크
  - Bank Switching
- [ ] Flash Driver
  - Sector Erase
  - Page Write
- [ ] Boot Config (EEPROM)
- [ ] 리셋 테스트

**목표**: Dual Bank 부팅 ✅

---

### Day 4: OTA + 통합
**오전 (4시간)**
- [ ] **MIKROE-3191 (Flash Click) 통합** ⭐ NEW!
  - QSPI2 드라이버 초기화
  - W25Q128JV SPI Flash 테스트 (16MB)
  - Zone Package 다운로드 버퍼 구현
- [ ] OTA Manager 구현
  - UDS 0x34 (RequestDownload)
  - UDS 0x36 (TransferData)
  - UDS 0x37 (RequestTransferExit)
- [ ] Inactive Bank에 쓰기
- [ ] CRC 검증
- [ ] Bank Switch

**오후 (4시간)**
- [ ] End-to-End OTA 테스트
  ```
  VMG → ZG: OTA Package (zone.bin 10MB)
  ZG: Download to MIKROE-3191 SPI Flash
  ZG: Parse & Extract zonal.bin
  ZG: Write zonal.bin to Bank B
  ZG: Verify
  ZG: Reboot
  ZG: Boot from Bank B ✅
  ```
- [ ] Rollback 테스트
- [ ] 문서 정리

**목표**: 전체 OTA 동작 ✅

---

## 🔄 OTA 구현 우선순위 (MIKROE-3191 통합)

### ⭐ 우선순위 1: Flash Programming (필수)
**Example**: `Flash_Programming_1_KIT_TC375_LK`
- **Phase 1: Package Transfer** - PFLASH Bank B 쓰기
- PSPR 사용하여 Bank A에서 실행하며 Bank B 프로그래밍
- EndInit Protection 관리

### ⭐ 우선순위 2: MIKROE-3191 SPI Flash Driver (필수) ⭐ NEW!
**하드웨어**: MIKROE-3191 (Flash 2 Click)
- **칩**: W25Q128JV (16MB SPI Flash)
- **인터페이스**: QSPI2 (mikroBUS 소켓)
- **용도**: Zone Package 임시 버퍼
- **Phase 1: Package Transfer** - 대용량 zone.bin (10MB) 저장

### ⭐ 우선순위 3: Flash ECC Error Injection (권장)
**Example**: `iLLD_TC375_ADS_Flash_ECC_Error_Injection`
- **Phase 2: Readiness Collection** - 무결성 검증
- Bank B 업데이트 후 ECC 에러 체크
- Rollback 메커니즘 테스트

### 우선순위 4: MPU Memory Protection (권장)
**Example**: `MPU_Memory_Protection_1_KIT_TC375_LK`
- **Phase 0: Secure Boot** - 부트로더 영역 보호
- Code Execution 제어

### 우선순위 5: BUS Memory Protection (선택)
**Example**: `iLLD_TC375_ADS_BUS_Memory_Protection_LiteKit`
- **Phase 1: Package Transfer** - 멀티코어 보호
- CPU 간 메모리 접근 제어

### 우선순위 6: Overlay (선택)
**Example**: `iLLD_TC375_ADS_OVERLAY_LITEKIT`
- **Phase 3: Activation** - 런타임 패치

---

## 🚀 Zonal Gateway OTA 구현 전략 (MIKROE-3191 통합) ⭐ NEW!

### 전체 아키텍처

```
┌────────────────────────────────────────────────────────────────┐
│                        VMG (Cloud)                             │
│            zone.bin (10MB) Generation                          │
│  ┌──────────────────────────────────────────────────────┐     │
│  │ Header (1KB)                                         │     │
│  │  - Magic: "ZONE"                                     │     │
│  │  - Version: 1.0.0                                    │     │
│  │  - Total Size: 10MB                                  │     │
│  │  - File Count: 4                                     │     │
│  │  - CRC32: 0xABCD1234                                 │     │
│  ├──────────────────────────────────────────────────────┤     │
│  │ File Table (256B)                                    │     │
│  │  Entry 1: zonal.bin  @ 0x400  (2MB)                 │     │
│  │  Entry 2: ecu1.bin   @ 0x200400 (3MB)               │     │
│  │  Entry 3: ecu2.bin   @ 0x500400 (2.5MB)             │     │
│  │  Entry 4: ecu3.bin   @ 0x770400 (2.5MB)             │     │
│  └──────────────────────────────────────────────────────┘     │
└────────────────────┬───────────────────────────────────────────┘
                     │ JSON over TCP (Ethernet)
                     │ zone.bin (10MB chunks)
                     ▼
┌────────────────────────────────────────────────────────────────┐
│              Zonal Gateway (TC375)                             │
│  ┌──────────────────────────────────────────────────────┐     │
│  │  Step 1: TCP Download (64KB chunks)                  │     │
│  │    lwIP → RAM Buffer (64KB)                          │     │
│  └────────────┬─────────────────────────────────────────┘     │
│               │                                                │
│               ▼                                                │
│  ┌──────────────────────────────────────────────────────┐     │
│  │  Step 2: MIKROE-3191 (Flash 2 Click)  ⭐ NEW!        │     │
│  │           W25Q128JV (16MB SPI Flash)                 │     │
│  │  ┌────────────────────────────────────────────┐     │     │
│  │  │ zone.bin (10MB)                            │     │     │
│  │  │  - Address: 0x000000 ~ 0xA00000            │     │     │
│  │  │  - Write Speed: ~1MB/s (10초)              │     │     │
│  │  │  - Read Speed: ~10MB/s (1초)               │     │     │
│  │  └────────────────────────────────────────────┘     │     │
│  │                                                      │     │
│  │  Hardware Connection (mikroBUS):                    │     │
│  │    TC375 QSPI2 ↔ MIKROE-3191                        │     │
│  │    P15.2 (SCLK)  → CLK                              │     │
│  │    P15.3 (MTSR)  → DI (MOSI)                        │     │
│  │    P15.6 (MRST)  ← DO (MISO)                        │     │
│  │    P15.1 (SLSO0) → CS                               │     │
│  │    3.3V → VCC, GND → GND                            │     │
│  └──────────────────────────────────────────────────────┘     │
│                                                                │
│  ┌──────────────────────────────────────────────────────┐     │
│  │  Step 3: Zone Package Parser                         │     │
│  │    1. Read Header from SPI Flash                     │     │
│  │    2. Validate Magic & CRC                           │     │
│  │    3. Parse File Table                               │     │
│  └────────────┬─────────────────────────────────────────┘     │
│               │                                                │
│               ▼                                                │
│  ┌──────────────────────────────────────────────────────┐     │
│  │  Step 4: File Extraction & Distribution              │     │
│  │                                                       │     │
│  │  4-1. zonal.bin (2MB)                                │     │
│  │       SPI Flash → RAM (64KB chunks)                  │     │
│  │              → PFLASH Bank B (0x80340000)            │     │
│  │                                                       │     │
│  │  4-2. ecu1.bin (3MB)                                 │     │
│  │       SPI Flash → RAM (64KB chunks)                  │     │
│  │              → DoIP/UDS to ECU1                      │     │
│  │                                                       │     │
│  │  4-3. ecu2.bin, ecu3.bin                             │     │
│  │       동일 방식으로 ECU2, ECU3에 전송                 │     │
│  └──────────────────────────────────────────────────────┘     │
└────────────────────────────────────────────────────────────────┘
```

### Phase별 상세 전략

#### Phase 0: MIKROE-3191 초기화 (시스템 부팅 시)
```c
// QSPI2 + W25Q128JV 초기화
void OTA_Init(void)
{
    // 1. QSPI2 Master 초기화 (mikroBUS)
    W25Q_Init();  // 10 MHz QSPI
    
    // 2. Flash Chip ID 확인
    uint8 mfr, dev;
    W25Q_ReadID(&mfr, &dev);
    // Expected: mfr=0xEF (Winbond), dev=0x17 (W25Q128)
    
    // 3. 전체 Erase (선택적, 초기화 시만)
    // W25Q_EraseChip();  // ~30초 소요
}
```

#### Phase 1: Package Transfer (VMG → SPI Flash)
```c
// VMG로부터 zone.bin 다운로드
void OTA_DownloadZonePackage(void)
{
    uint8 buffer[65536];  // 64KB RAM 버퍼
    uint32 spiFlashAddr = 0x000000;
    uint32 totalSize = 10 * 1024 * 1024;  // 10MB
    uint32 received = 0;
    
    while (received < totalSize)
    {
        // 1. VMG로부터 64KB 수신 (TCP)
        uint32 chunkSize = tcp_receive(buffer, 65536);
        
        // 2. SPI Flash에 쓰기 (페이지 단위)
        for (uint32 i = 0; i < chunkSize; i += 256)
        {
            W25Q_WritePage(spiFlashAddr + i, &buffer[i], 256);
        }
        
        spiFlashAddr += chunkSize;
        received += chunkSize;
        
        // 3. 진행률 표시
        uint8 progress = (received * 100) / totalSize;
        UART_Printf("OTA Download: %d%%\r\n", progress);
    }
    
    UART_Printf("OTA Download Complete: %d MB\r\n", received / 1024 / 1024);
}
```

**성능 분석**:
- TCP 다운로드: ~10초 (100Mbps Ethernet, 10MB)
- SPI Flash 쓰기: ~10초 (1MB/s, 10MB)
- **총 소요시간: ~20초**

#### Phase 2: Readiness Collection (무결성 검증)
```c
// Zone Package 검증
ZonePackageError_t OTA_ValidatePackage(void)
{
    // 1. Header 읽기
    ZonePackageHeader_t header;
    W25Q_Read(0x000000, (uint8*)&header, sizeof(header));
    
    // 2. Magic Number 확인
    if (header.magic != 0x5A4F4E45)  // "ZONE"
    {
        return ZONE_ERR_MAGIC;
    }
    
    // 3. CRC32 검증 (전체 패키지)
    uint32 calculatedCRC = 0;
    uint8 buffer[4096];
    uint32 addr = sizeof(ZonePackageHeader_t);
    
    while (addr < header.totalSize)
    {
        uint32 readSize = (header.totalSize - addr > 4096) ? 4096 : (header.totalSize - addr);
        W25Q_Read(addr, buffer, readSize);
        calculatedCRC = crc32_update(calculatedCRC, buffer, readSize);
        addr += readSize;
    }
    
    if (calculatedCRC != header.crc32)
    {
        return ZONE_ERR_CRC;
    }
    
    return ZONE_OK;
}
```

#### Phase 3: Activation (파일 추출 및 배포)
```c
// zonal.bin 추출 → PFLASH Bank B
void OTA_UpdateZonalGateway(void)
{
    // 1. File Entry 찾기
    ZoneFileEntry_t entry;
    if (ZonePackage_FindFile("ZONAL_GATEWAY", &entry) != ZONE_OK)
    {
        return;
    }
    
    // 2. Bank B Erase
    Flash_EraseBank(BANK_B_START, BANK_B_SIZE);
    
    // 3. SPI Flash → PFLASH (64KB 청크 단위)
    uint8 buffer[65536];
    uint32 spiFlashAddr = entry.offset;
    uint32 pflashAddr = BANK_B_START;
    uint32 remaining = entry.size;
    
    while (remaining > 0)
    {
        uint32 chunkSize = (remaining > 65536) ? 65536 : remaining;
        
        // SPI Flash에서 읽기
        W25Q_FastRead(spiFlashAddr, buffer, chunkSize);
        
        // PFLASH에 쓰기 (32 Byte 페이지 단위)
        for (uint32 i = 0; i < chunkSize; i += 32)
        {
            Flash_WritePage(pflashAddr + i, &buffer[i], 32);
        }
        
        spiFlashAddr += chunkSize;
        pflashAddr += chunkSize;
        remaining -= chunkSize;
    }
    
    // 4. 검증
    uint32 crc = Flash_CalculateCRC(BANK_B_START, entry.size);
    if (crc == entry.crc32)
    {
        // 5. BMHD 업데이트 (Boot to Bank B)
        Update_BMHD_STAD(BANK_B_START);
    }
}

// ECU 펌웨어 스트리밍 전송
void OTA_UpdateECU(const char* ecuName)
{
    // 1. File Entry 찾기
    ZoneFileEntry_t entry;
    if (ZonePackage_FindFile(ecuName, &entry) != ZONE_OK)
    {
        return;
    }
    
    // 2. DoIP로 스트리밍 전송
    uint8 buffer[4096];  // 4KB UDS 전송 단위
    uint32 spiFlashAddr = entry.offset;
    uint32 remaining = entry.size;
    
    // UDS RequestDownload
    DoIP_UDS_RequestDownload(ecuName, entry.size);
    
    while (remaining > 0)
    {
        uint32 chunkSize = (remaining > 4096) ? 4096 : remaining;
        
        // SPI Flash에서 읽기
        W25Q_FastRead(spiFlashAddr, buffer, chunkSize);
        
        // UDS TransferData
        DoIP_UDS_TransferData(ecuName, buffer, chunkSize);
        
        spiFlashAddr += chunkSize;
        remaining -= chunkSize;
    }
    
    // UDS RequestTransferExit
    DoIP_UDS_RequestTransferExit(ecuName);
}
```

**메모리 효율**:
- RAM 사용량: 64KB (버퍼) + 약 20KB (프로토콜 스택) = **~84KB**
- SPI Flash 사용량: 10MB (zone.bin) / 16MB = **62.5%**
- PFLASH 사용량: 2MB (zonal.bin) / 2.88MB (Bank B) = **69.4%**

#### Phase 4: Result Report
```c
// OTA 결과 보고
void OTA_ReportResult(void)
{
    OTAResult_t result;
    
    // 1. Self Test
    result.zonalGW.status = (Get_Current_Bank() == BANK_B) ? OTA_SUCCESS : OTA_FAILED;
    result.zonalGW.version = Get_Firmware_Version();
    
    // 2. ECU Result 수집
    for (int i = 0; i < 3; i++)
    {
        result.ecu[i].status = DoIP_UDS_ReadDID(ecuNames[i], DID_OTA_STATUS);
        result.ecu[i].version = DoIP_UDS_ReadDID(ecuNames[i], DID_SW_VERSION);
    }
    
    // 3. VMG로 전송
    JSON_SendOTAResult(&result);
}
```

---

## 💾 MIKROE-3191 상세 사양

### 하드웨어
| 항목 | 사양 |
|------|------|
| **제품명** | MIKROE-3191 (Flash 2 Click) |
| **Flash 칩** | W25Q128JV (Winbond) |
| **용량** | 16MB (128 Mbit) |
| **인터페이스** | SPI (QSPI 호환) |
| **전원** | 3.3V |
| **커넥터** | mikroBUS 소켓 |
| **최대 속도** | 104 MHz (QSPI 모드) |

### 성능
| 작업 | 속도 | 비고 |
|------|------|------|
| **Read** | ~10 MB/s | Fast Read (0x0B) |
| **Page Program** | ~256 Bytes/1ms | 256 Byte 페이지 |
| **Sector Erase (4KB)** | ~45 ms | 최소 단위 |
| **Block Erase (64KB)** | ~150 ms | 권장 단위 |
| **Chip Erase** | ~30 s | 전체 삭제 |

### 메모리 맵
```
W25Q128JV (16MB)
┌───────────────────────────────────────┐
│ 0x000000 ~ 0x9FFFFF  Zone Package     │ 10MB
│                      (10MB)            │
├───────────────────────────────────────┤
│ 0xA00000 ~ 0xFFFFFF  Reserved         │ 6MB
│                      (Future Use)      │
└───────────────────────────────────────┘
```

---

## 📚 코드 구조

```
Zonal_Gateway/
├── PROTOTYPE_SKETCH.md          ← 이 문서
├── README.md
├── Configurations/
│   ├── Debug/
│   └── Ifx_Cfg_Ssw.c
├── Bootloader/
│   ├── ssw/
│   │   ├── ssw_main.c           (Stage 1)
│   │   └── ssw_linker.ld
│   ├── stage2/
│   │   ├── bootloader_main.c    (Stage 2)
│   │   ├── bootloader_a_linker.ld
│   │   └── bootloader_b_linker.ld
│   └── common/
│       ├── boot_common.h
│       └── flash_driver.c
├── Application/
│   ├── main.c
│   ├── linker_app_a.ld
│   ├── linker_app_b.ld
│   └── src/
│       ├── zonal_gateway.c
│       ├── doip_server.c
│       ├── doip_client.c
│       ├── uds_handler.c
│       ├── ota_manager.c
│       └── vci_collector.c
├── Network/
│   ├── lwip_config/
│   │   └── lwipopts.h
│   └── ethernet_driver.c
└── RTOS/
    ├── FreeRTOSConfig.h
    └── tasks/
        ├── task_network_rx.c
        ├── task_doip_server.c
        ├── task_doip_client.c
        ├── task_vci_collection.c
        ├── task_ota_manager.c
        └── task_watchdog.c
```

---

## ⚠️ 제약사항 및 주의사항

### 메모리 제약
- ✅ Flash: 600 KB / 2.88 MB = 21% 사용 (여유 충분)
- ✅ RAM: 190 KB / 512 KB = 37% 사용 (여유 충분)
- ⚠️ Application은 2.88 MB 내에서 개발 필요

### 성능 제약
- ⚠️ lwIP는 단일 스레드 (lwIP Core Lock 사용)
- ⚠️ Flash Write 느림 (~5ms/page)
- ⚠️ OTA 중 네트워크 응답 지연 가능 (RTOS로 해결)

### 안전 제약
- 🔴 **SSW는 절대 업데이트 금지** (Brick 위험)
- ⚠️ Bootloader A, B 동시 업데이트 금지 (한 번에 하나씩)
- ⚠️ Flash Write 중 전원 차단 주의 (Transaction 보호)

### 개발 제약
- ⚠️ TC375 하드웨어 의존 (디버깅 시간 소요)
- ⚠️ JTAG 디버거 필요
- ⚠️ Ethernet PHY 초기화 이슈 가능

---

## 🔧 빌드 및 플래싱

### 빌드 순서
```bash
# 1. SSW (Stage 1) - 한 번만
cd Bootloader/ssw
tricore-gcc -c ssw_main.c -o ssw_main.o
tricore-ld -T ssw_linker.ld -o ssw.elf ssw_main.o
tricore-objcopy -O ihex ssw.elf ssw.hex

# 2. Bootloader A
cd Bootloader/stage2
tricore-gcc -DBOOT_A -c bootloader_main.c -o bootloader_a.o
tricore-ld -T bootloader_a_linker.ld -o bootloader_a.elf bootloader_a.o
tricore-objcopy -O ihex bootloader_a.elf bootloader_a.hex

# 3. Bootloader B (처음엔 A와 동일)
tricore-gcc -DBOOT_B -c bootloader_main.c -o bootloader_b.o
tricore-ld -T bootloader_b_linker.ld -o bootloader_b.elf bootloader_b.o
tricore-objcopy -O ihex bootloader_b.elf bootloader_b.hex

# 4. Application A
cd Application
tricore-gcc -DAPP_A -c main.c -o app_a.o
tricore-ld -T linker_app_a.ld -o app_a.elf app_a.o
tricore-objcopy -O ihex app_a.elf app_a.hex
```

### 플래싱 (최초)
```bash
# Infineon Memtool 사용
memtool write 0x80000100 ssw.hex
memtool write 0x80021000 bootloader_a.hex
memtool write 0x80041000 bootloader_b.hex
memtool write 0x80061000 app_a.hex
```

---

## 📊 성능 목표

| 항목 | 목표 | 측정 방법 |
|-----|------|----------|
| **DoIP 응답 시간** | < 50ms | 타이머 측정 |
| **Heartbeat 주기** | 10s ± 100ms | 로그 분석 |
| **OTA Download 속도** | > 100 KB/s | 진행률 측정 |
| **Flash Write 속도** | ~5ms/page | 타이머 측정 |
| **RAM 사용량** | < 256 KB | 런타임 체크 |
| **CPU 사용률** | < 70% | Task 통계 |

---

## 🐛 디버깅 전략

### UART 로그 레벨
```c
#define LOG_ERROR   0  // 항상 출력
#define LOG_WARN    1  // 경고
#define LOG_INFO    2  // 정보 (기본)
#define LOG_DEBUG   3  // 디버그 (상세)
```

### 주요 로그 포인트
```c
[BOOT] SSW: Selecting Bootloader A
[BOOT] Boot A: Verifying App A... CRC OK
[BOOT] Boot A: Jumping to App A @ 0x80061000
[APP] Application A v1.0.0 started
[DOIP] Server listening on :13400
[DOIP] Client connected to VMG 192.168.1.1:13400
[OTA] Downloading to Bank B... 45%
[OTA] Verify OK. Switching to Bank B.
[BOOT] Reboot: App B v1.1.0
```

---

## ✅ 검증 체크리스트

### Day 1 검증
- [ ] Ethernet Link Up
- [ ] Ping 응답
- [ ] TCP Socket 통신

### Day 2 검증
- [ ] DoIP Routing Activation 성공
- [ ] UDS 0x22로 VIN 읽기
- [ ] Heartbeat 10초마다 전송

### Day 3 검증
- [ ] SSW에서 Bootloader 선택
- [ ] Bootloader에서 Application 선택
- [ ] Bank A/B 전환 성공

### Day 4 검증
- [ ] OTA Download 성공
- [ ] CRC 검증 통과
- [ ] Reboot 후 새 펌웨어 실행
- [ ] Rollback 동작 확인

---

## 📝 참고 문서

### 내부 문서
- `C:\Users\user\Desktop\VMGandECUs\docs\architecture\system_overview.md`
- `C:\Users\user\Desktop\VMGandECUs\docs\system\zonal_gateway_architecture.md`
- `C:\Users\user\Desktop\VMGandECUs\docs\ota\ota_scenario_detailed.md`
- `C:\Users\user\Desktop\VMGandECUs\docs\network\ISO_13400_specification.md`
- `C:\Users\user\Desktop\VMGandECUs\docs\bootloader\tc375_bootloader_guide.md`

### 외부 표준
- ISO 13400: Diagnostics over IP (DoIP)
- ISO 14229: Unified Diagnostic Services (UDS)
- AURIX TC375 User Manual
- FreeRTOS Documentation
- lwIP Documentation

---

## 🎯 다음 단계

### 즉시 시작
1. AURIX Development Studio 프로젝트 생성
2. FreeRTOS 템플릿 선택
3. lwIP 통합
4. Ethernet PHY 초기화

### 향후 확장 (4일 이후)
- [ ] UDP Discovery 구현
- [ ] JSON Server (포트 8765)
- [ ] PQC Signature 검증
- [ ] mbedTLS 통합 (선택적)
- [ ] HSM 활용 (선택적)
- [ ] Delta OTA (선택적)
- [ ] 압축 전송 (선택적)

---

**마지막 업데이트**: 2025-11-01  
**작성자**: AI Assistant + User  
**상태**: 설계 완료, 구현 준비 완료 ✅

