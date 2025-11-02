# TC375 Ethernet + lwIP - Complete Reference

> 출처: Infineon AURIX Code Examples - Ethernet_1_KIT_TC375_LK  
> 목적: Zonal Gateway Ethernet 통신 구현을 위한 완전한 참고 자료

---

## 📋 목차

1. [TC375 Ethernet 하드웨어 구조](#1-tc375-ethernet-하드웨어-구조)
2. [lwIP 스택 개요](#2-lwip-스택-개요)
3. [프로젝트 구조](#3-프로젝트-구조)
4. [lwIP 설정 (lwipopts.h)](#4-lwip-설정-lwiopth)
5. [GETH 초기화](#5-geth-초기화)
6. [PHY 드라이버 (DP83825I)](#6-phy-드라이버-dp83825i)
7. [lwIP 포팅 레이어](#7-lwip-포팅-레이어)
8. [STM 타이머 통합](#8-stm-타이머-통합)
9. [Echo 서버 구현 (TCP RAW API)](#9-echo-서버-구현-tcp-raw-api)
10. [DHCP 구성](#10-dhcp-구성)
11. [UART 디버깅](#11-uart-디버깅)
12. [실전 코드 예제](#12-실전-코드-예제)
13. [요약 및 체크리스트](#13-요약-및-체크리스트)

---

## 1. TC375 Ethernet 하드웨어 구조

### 1.1 OSI 모델과 TC375 구현

```
┌─────────────────────┬─────────────────────────┐
│  OSI Layer          │  TC375 구현              │
├─────────────────────┼─────────────────────────┤
│ Layer 7: Application│  Echo Server (Echo.c)    │  ← 애플리케이션
├─────────────────────┼─────────────────────────┤
│ Layer 4: Transport  │  lwIP: TCP/UDP           │
├─────────────────────┼─────────────────────────┤
│ Layer 3: Network    │  lwIP: IP, ICMP, DHCP    │  ← lwIP 소프트웨어
├─────────────────────┼─────────────────────────┤
│ Layer 2: Data Link  │  GETH (MAC) + lwIP ARP   │
├─────────────────────┼─────────────────────────┤
│ Layer 1: Physical   │  PHY DP83825I (RMII)     │  ← 하드웨어
└─────────────────────┴─────────────────────────┘
```

### 1.2 GETH (Gigabit Ethernet Media Access Controller)

**역할**: IEEE 802.3 표준에 따른 MAC 계층 구현

**주요 기능**:
- MAC 주소 필터링
- CRC 계산/검증
- Frame 송수신 (DMA 사용)
- MDIO 인터페이스 (PHY 제어)

**TC375 GETH 특징**:
- Gigabit Ethernet 지원 (10/100/1000 Mbps)
- IEEE 1588 PTP (Precision Time Protocol) 지원
- RMII/MII/GMII/RGMII 인터페이스
- 8개의 DMA 채널
- Hardware Checksum Offload

### 1.3 PHY (Physical Layer Transceiver)

**사용 PHY**: Texas Instruments DP83825I

**특징**:
- 10/100 Mbps Ethernet PHY
- RMII 인터페이스 (Reduced MII)
- Auto-negotiation
- MDIO 관리 인터페이스
- 3.3V 단일 전원

**RMII 핀 연결** (TC375 Lite Kit 기준):

```c
// Configuration.h에서 정의됨
#define ETH_CRSDIV_PIN    IfxGeth_CRSDVA_P11_11_IN   // Carrier Sense / Data Valid
#define ETH_REFCLK_PIN    IfxGeth_REFCLKA_P11_12_IN  // 50MHz Reference Clock
#define ETH_TXEN_PIN      IfxGeth_TXEN_P11_6_OUT     // Transmit Enable
#define ETH_RXD0_PIN      IfxGeth_RXD0A_P11_10_IN    // Receive Data 0
#define ETH_RXD1_PIN      IfxGeth_RXD1A_P11_9_IN     // Receive Data 1
#define ETH_MDC_PIN       IfxGeth_MDC_P21_2_OUT      // Management Data Clock
#define ETH_MDIO_PIN      IfxGeth_MDIO_P21_3_INOUT   // Management Data I/O
#define ETH_TXD0_PIN      IfxGeth_TXD0_P11_3_OUT     // Transmit Data 0
#define ETH_TXD1_PIN      IfxGeth_TXD1_P11_2_OUT     // Transmit Data 1
```

**RMII vs MII 비교**:

| 항목 | RMII | MII |
|------|------|-----|
| 데이터 폭 | 2-bit | 4-bit |
| 클럭 속도 | 50 MHz | 25 MHz (100Mbps) |
| 핀 수 | 7개 (신호) | 16개 (신호) |
| 최대 속도 | 100 Mbps | 100 Mbps |

---

## 2. lwIP 스택 개요

### 2.1 lwIP란?

**lwIP** (Lightweight IP): 임베디드 시스템을 위한 소형 TCP/IP 프로토콜 스택

**특징**:
- 메모리 사용량 최소화 (RAM: ~40KB, ROM: ~30KB)
- RTOS 필수 아님 (Bare-metal 지원)
- 표준 TCP/IP 프로토콜 지원
- BSD 소켓 API 호환 (선택)

### 2.2 lwIP API 종류

#### 2.2.1 RAW API (Native API)

**특징**:
- lwIP의 네이티브 API
- 최고 성능, 최소 메모리
- 이벤트 기반 (콜백)
- **Thread-safe 아님**

**사용 예제**:
```c
tcp_pcb *pcb = tcp_new();
tcp_bind(pcb, IP_ADDR_ANY, 80);
pcb = tcp_listen(pcb);
tcp_accept(pcb, accept_callback);
```

#### 2.2.2 Netconn API

**특징**:
- RAW API 위에 구축된 순차적 API
- RTOS 필수
- Thread-safe
- 메시지 큐 사용

#### 2.2.3 Socket API

**특징**:
- BSD 소켓 API 호환
- RTOS 필수
- 가장 높은 메모리 사용량
- 이식성 최고

**이 예제에서 사용**: **RAW API** (NO_SYS=1, Bare-metal)

---

## 3. 프로젝트 구조

### 3.1 디렉터리 구조

```
Ethernet_1_KIT_TC375_LK/
├── Cpu0_Main.c                      # 메인 로직 (Ethernet 초기화, 메인 루프)
├── Echo.c/h                         # Echo 서버 구현 (TCP RAW API)
├── Configurations/
│   ├── Configuration.h              # 핀 정의, 매크로
│   ├── ConfigurationIsr.h           # ISR 우선순위
│   └── lwipopts.h                   # lwIP 설정
├── Libraries/
│   ├── Ethernet/
│   │   ├── lwip/                    # lwIP 소스 코드 (2.1.3)
│   │   │   ├── src/                 # lwIP 핵심 (core, api, netif)
│   │   │   └── port/                # TC375 포팅 레이어
│   │   │       ├── include/
│   │   │       │   ├── Ifx_Lwip.h   # lwIP 포팅 헤더
│   │   │       │   ├── Ifx_Netif.h  # netif 함수
│   │   │       │   └── arch/        # 아키텍처 정의
│   │   │       └── src/
│   │   │           ├── Ifx_Lwip.c   # lwIP 포팅 구현
│   │   │           └── netif.c      # netif 구현
│   │   └── Phy_Dp83825i/            # PHY 드라이버
│   │       ├── IfxGeth_Phy_Dp83825i.c
│   │       └── IfxGeth_Phy_Dp83825i.h
│   ├── iLLD/                        # Infineon Low-Level Drivers
│   │   └── TC37A/Tricore/Geth/      # GETH 드라이버
│   └── UART/                        # UART 로깅
│       ├── UART_Logging.c
│       └── UART_Logging.h
└── Lcf_*.lsl                        # Linker Script (TASKING/GCC)
```

### 3.2 주요 파일 역할

| 파일 | 역할 |
|------|------|
| `Cpu0_Main.c` | GETH 활성화, lwIP 초기화, STM 타이머, 메인 루프 |
| `Echo.c/h` | TCP Echo 서버 (RAW API 사용) |
| `Ifx_Lwip.c/h` | lwIP 포팅 레이어 (초기화, 타이머, netif 연결) |
| `Ifx_Netif.c/h` | netif 구현 (송수신, GETH 연결) |
| `IfxGeth_Phy_Dp83825i.c/h` | PHY 드라이버 (MDIO 통신, Link 상태) |
| `lwipopts.h` | lwIP 설정 (프로토콜, 메모리, 디버그) |

---

## 4. lwIP 설정 (lwipopts.h)

### 4.1 핵심 설정

```c
// ========== 운영 모드 ==========
#define NO_SYS                  1       // Bare-metal 모드 (RTOS 없음)
#define SYS_LIGHTWEIGHT_PROT    0       // Inter-task protection 비활성화

// ========== API 선택 ==========
#define LWIP_NETCONN            0       // Netconn API 비활성화
#define LWIP_SOCKET             0       // Socket API 비활성화

// ========== 메모리 설정 ==========
#define MEM_ALIGNMENT           4       // 32-bit 정렬
#define MEM_SIZE                (25 * 1024)  // Heap 크기: 25KB

// ========== 프로토콜 설정 ==========
#define LWIP_DHCP               1       // DHCP 클라이언트 활성화
#define LWIP_NETIF_HOSTNAME     1       // DHCP에서 hostname 전송

// ========== Ethernet 설정 ==========
#define ETH_PAD_SIZE            2       // Ethernet 헤더 앞에 2바이트 패딩
                                        // → Payload가 4바이트 정렬되도록

// ========== 디버깅 ==========
#define LWIP_DEBUG                      // lwIP 디버깅 활성화
#define DHCP_DEBUG              LWIP_DBG_OFF
#define NETIF_DEBUG             LWIP_DBG_ON
```

### 4.2 프로토콜 기본 설정 (lwip/opt.h에서 정의됨)

```c
// TCP 활성화 (기본값 = 1)
#define LWIP_TCP                1

// UDP 활성화 (기본값 = 1)
#define LWIP_UDP                1

// ICMP 활성화 (기본값 = 1, ping 필요)
#define LWIP_ICMP               1

// ARP 활성화 (기본값 = 1, Ethernet 필수)
#define LWIP_ARP                1
```

### 4.3 타이머 주기 (lwip/opt.h에서 정의됨)

```c
// ARP 테이블 업데이트 주기
#define ARP_TMR_INTERVAL        1000    // 1초

// TCP Fast 타이머 (재전송 확인)
#define TCP_FAST_INTERVAL       250     // 250ms

// TCP Slow 타이머 (Keep-alive, Persist)
#define TCP_SLOW_INTERVAL       500     // 500ms

// DHCP Coarse 타이머
#define DHCP_COARSE_TIMER_MSECS 60000   // 60초

// DHCP Fine 타이머
#define DHCP_FINE_TIMER_MSECS   500     // 500ms
```

---

## 5. GETH 초기화

### 5.1 GETH 모듈 활성화

```c
#include "IfxGeth_Eth.h"

// Cpu0_Main.c
void core0_main(void)
{
    // ...

    // GETH 모듈 활성화
    IfxGeth_enableModule(&MODULE_GETH);

    // ...
}
```

**`IfxGeth_enableModule()` 역할**:
- GETH 클럭 활성화
- 모듈 리셋 해제
- 기본 레지스터 초기화

### 5.2 MAC 주소 설정

```c
// MAC 주소 정의
eth_addr_t ethAddr;
ethAddr.addr[0] = 0xDE;
ethAddr.addr[1] = 0xAD;
ethAddr.addr[2] = 0xBE;
ethAddr.addr[3] = 0xEF;
ethAddr.addr[4] = 0xFE;
ethAddr.addr[5] = 0xED;

// lwIP 초기화 시 MAC 주소 전달
Ifx_Lwip_init(ethAddr);
```

**MAC 주소 할당 가이드**:
- **Locally Administered Address** 사용 권장:
  - 첫 바이트의 2번째 비트를 1로 설정
  - 예: `0xDE` = `11011110` (2번째 비트 = 1)
- **Universally Administered Address**: IEEE에서 공식 할당 (제조사용)

### 5.3 GETH 전체 초기화 플로우

```
Power-On
    |
    v
IfxGeth_enableModule()         ← GETH 클럭/리셋
    |
    v
Ifx_Lwip_init()                ← lwIP 스택 초기화
    |
    +---> lwip_init()          ← lwIP 내부 초기화
    |
    +---> IfxGeth_Eth_init()   ← GETH 하드웨어 설정
    |       |
    |       +---> Pin 설정 (RMII 핀)
    |       +---> DMA 설정 (Descriptor, Buffer)
    |       +---> MAC 설정 (주소, 필터)
    |       +---> PHY 초기화 (DP83825I)
    |
    +---> netif_add()          ← netif 등록
            |
            +---> ifx_netif_init()  ← netif 초기화 콜백
            +---> dhcp_start()      ← DHCP 시작
```

---

## 6. PHY 드라이버 (DP83825I)

### 6.1 PHY 초기화

```c
// IfxGeth_Phy_Dp83825i.c (예제)
uint32 IfxGeth_Eth_Phy_Dp83825i_init(void)
{
    uint32 phyStatus = 0;
    uint32 regData = 0;

    // 1. PHY Reset
    IfxGet_Eth_Phy_Dp83825i_reset();

    // 2. PHY ID 읽기 (확인용)
    IfxGeth_Eth_Phy_Dp83825i_read_mdio_reg(PHY_ADDR, PHY_REG_ID1, &regData);
    // Expected: 0x2000

    // 3. Auto-Negotiation 활성화
    IfxGeth_Eth_Phy_Dp83825i_write_mdio_reg(PHY_ADDR, PHY_REG_BMCR, 
                                              BMCR_AUTONEG | BMCR_RESTART_AUTONEG);

    // 4. Link 상태 확인 (폴링)
    while (IfxGeth_Eth_Phy_Dp83825i_link_status() == 0)
    {
        // Wait for link up
    }

    IfxGeth_Eth_Phy_Dp83825i_iPhyInitDone = 1;

    return phyStatus;
}
```

### 6.2 MDIO 통신

**MDIO** (Management Data Input/Output): PHY 레지스터 읽기/쓰기 인터페이스

```c
// MDIO Read
void IfxGeth_Eth_Phy_Dp83825i_read_mdio_reg(uint32 phyAddr, 
                                              uint32 regAddr, 
                                              uint32 *pdata)
{
    IfxGeth_Eth *geth = IfxGeth_get();
    
    // iLLD API 사용
    IfxGeth_Eth_readMdio(geth, phyAddr, regAddr, pdata);
}

// MDIO Write
void IfxGeth_Eth_Phy_Dp83825i_write_mdio_reg(uint32 phyAddr, 
                                               uint32 regAddr, 
                                               uint32 data)
{
    IfxGeth_Eth *geth = IfxGeth_get();
    
    // iLLD API 사용
    IfxGeth_Eth_writeMdio(geth, phyAddr, regAddr, data);
}
```

### 6.3 PHY 주요 레지스터

| 레지스터 주소 | 이름 | 설명 |
|-------------|------|------|
| 0x00 | BMCR (Basic Mode Control) | Auto-negotiation, Reset, Loopback |
| 0x01 | BMSR (Basic Mode Status) | Link status, Auto-negotiation status |
| 0x02 | PHYIDR1 | PHY Identifier 1 (0x2000) |
| 0x03 | PHYIDR2 | PHY Identifier 2 |
| 0x04 | ANAR (Auto-Negotiation Advertisement) | 10/100, Full/Half duplex |
| 0x05 | ANLPAR (Auto-Negotiation Link Partner) | 상대방 능력 |

### 6.4 Link 상태 확인

```c
uint32 IfxGeth_Eth_Phy_Dp83825i_link_status(void)
{
    uint32 regData = 0;
    
    // BMSR 레지스터 읽기
    IfxGeth_Eth_Phy_Dp83825i_read_mdio_reg(PHY_ADDR, PHY_REG_BMSR, &regData);
    
    // Link Status 비트 확인 (bit 2)
    return (regData & 0x0004) ? 1 : 0;
}
```

---

## 7. lwIP 포팅 레이어

### 7.1 포팅 레이어 구조

```
┌─────────────────────────────────────┐
│      lwIP Core (독립적)              │
│  (TCP/IP 프로토콜 구현)              │
└──────────────┬──────────────────────┘
               │ 인터페이스
┌──────────────▼──────────────────────┐
│      포팅 레이어 (TC375 특화)         │
│  ├─ Ifx_Lwip.c/h   (초기화, 타이머)  │
│  ├─ Ifx_Netif.c/h  (송수신)          │
│  └─ arch/cc.h      (컴파일러 정의)   │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      TC375 Hardware                  │
│  ├─ GETH (iLLD)                      │
│  ├─ PHY (DP83825I)                   │
│  └─ STM (타이머)                     │
└──────────────────────────────────────┘
```

### 7.2 Ifx_Lwip_init()

**역할**: lwIP 스택 초기화 및 netif 등록

```c
// Ifx_Lwip.c
void Ifx_Lwip_init(eth_addr_t ethAddr)
{
    Ifx_Lwip *lwip = &g_Lwip;

    // 1. lwIP Core 초기화
    lwip_init();

    // 2. MAC 주소 설정
    memcpy(&lwip->eth_addr, &ethAddr, sizeof(eth_addr_t));

    // 3. netif 등록
    netif_add(&lwip->netif,              // netif 구조체
              IP_ADDR_ANY,               // IP 주소 (DHCP로 할당받을 예정)
              IP_ADDR_ANY,               // Netmask
              IP_ADDR_ANY,               // Gateway
              NULL,                      // state (사용자 데이터)
              ifx_netif_init,            // 초기화 콜백
              ethernet_input);           // 입력 처리 함수

    // 4. netif를 기본으로 설정
    netif_set_default(&lwip->netif);

    // 5. netif 활성화
    netif_set_up(&lwip->netif);

    // 6. DHCP 시작
#if LWIP_DHCP
    dhcp_start(&lwip->netif);
    netif_set_hostname(&lwip->netif, BOARDNAME);
#endif

    // 7. 타이머 초기화
    lwip->timer.arp = 0;
    lwip->timer.dhcp_coarse = 0;
    lwip->timer.dhcp_fine = 0;
    lwip->timer.tcp_fast = 0;
    lwip->timer.tcp_slow = 0;
    lwip->timer.link = 0;
    lwip->timerFlags = 0;
}
```

### 7.3 ifx_netif_init()

**역할**: netif 초기화 (GETH 설정)

```c
// netif.c
err_t ifx_netif_init(struct netif *netif)
{
    Ifx_Lwip *lwip = &g_Lwip;

    // 1. netif 이름 설정
    netif->name[0] = 'e';
    netif->name[1] = 'n';

    // 2. netif 콜백 함수 설정
    netif->output = etharp_output;   // IP 패킷 출력 (ARP 포함)
    netif->linkoutput = ifx_netif_linkoutput;  // Ethernet 프레임 출력

    // 3. MAC 주소 설정
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    memcpy(netif->hwaddr, lwip->eth_addr.addr, ETHARP_HWADDR_LEN);

    // 4. netif 플래그 설정
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | 
                   NETIF_FLAG_ETHARP | 
                   NETIF_FLAG_ETHERNET |
                   NETIF_FLAG_LINK_UP;

    // 5. GETH 하드웨어 초기화
    IfxGeth_Eth_Config gethConfig;
    IfxGeth_Eth_initModuleConfig(&gethConfig, &MODULE_GETH);

    // MAC 주소 설정
    for (int i = 0; i < 6; i++)
    {
        gethConfig.macAddress.byte[i] = lwip->eth_addr.addr[i];
    }

    // RMII 핀 설정
    gethConfig.pins.crsDiv = &ETH_CRSDIV_PIN;
    gethConfig.pins.refClk = &ETH_REFCLK_PIN;
    gethConfig.pins.txEnable = &ETH_TXEN_PIN;
    gethConfig.pins.rxData0 = &ETH_RXD0_PIN;
    gethConfig.pins.rxData1 = &ETH_RXD1_PIN;
    gethConfig.pins.mdc = &ETH_MDC_PIN;
    gethConfig.pins.mdio = &ETH_MDIO_PIN;
    gethConfig.pins.txData0 = &ETH_TXD0_PIN;
    gethConfig.pins.txData1 = &ETH_TXD1_PIN;

    // DMA 버퍼 설정
    gethConfig.txBuffer = (uint32)channel0TxBuffer1;
    gethConfig.rxBuffer = (uint32)channel0RxBuffer1;

    // GETH 초기화
    IfxGeth_Eth_init(&g_IfxGeth, &gethConfig);

    // 6. PHY 초기화
    IfxGeth_Eth_Phy_Dp83825i_init();

    return ERR_OK;
}
```

### 7.4 패킷 송신 (ifx_netif_linkoutput)

```c
// netif.c
err_t ifx_netif_linkoutput(struct netif *netif, struct pbuf *p)
{
    IfxGeth_Eth *geth = IfxGeth_get();
    err_t result = ERR_OK;

    // pbuf 체인을 단일 버퍼로 복사
    uint32 totalLen = 0;
    uint8 *txBuffer = geth->txBuffer;  // DMA 버퍼

    for (struct pbuf *q = p; q != NULL; q = q->next)
    {
        memcpy(&txBuffer[totalLen], q->payload, q->len);
        totalLen += q->len;
    }

    // GETH로 전송
    if (IfxGeth_Eth_sendFrame(geth, txBuffer, totalLen) == FALSE)
    {
        result = ERR_IF;
    }

    return result;
}
```

### 7.5 패킷 수신 (ifx_netif_input)

```c
// netif.c
err_t ifx_netif_input(struct netif *netif)
{
    IfxGeth_Eth *geth = IfxGeth_get();
    uint32 rxLen = 0;
    uint8 *rxBuffer = geth->rxBuffer;  // DMA 버퍼

    // GETH에서 수신 확인
    if (IfxGeth_Eth_isRxDataAvailable(geth))
    {
        // 수신 데이터 읽기
        rxLen = IfxGeth_Eth_receiveFrame(geth, rxBuffer, IFXGETH_MAX_RX_BUFFER_SIZE);

        if (rxLen > 0)
        {
            // pbuf 할당
            struct pbuf *p = pbuf_alloc(PBUF_RAW, rxLen, PBUF_POOL);

            if (p != NULL)
            {
                // 데이터 복사
                pbuf_take(p, rxBuffer, rxLen);

                // lwIP에 전달
                if (netif->input(p, netif) != ERR_OK)
                {
                    pbuf_free(p);
                }
            }
        }
    }

    return ERR_OK;
}
```

---

## 8. STM 타이머 통합

### 8.1 STM (System Timer Module)

**역할**: lwIP 타이머 업데이트용 주기적 인터럽트 생성

**주기**: 1ms (lwIP 타이머 해상도)

### 8.2 STM 초기화

```c
// Cpu0_Main.c
void core0_main(void)
{
    // ...

    IfxStm_CompareConfig stmCompareConfig;

    // 기본 설정 초기화
    IfxStm_initCompareConfig(&stmCompareConfig);

    // 설정
    stmCompareConfig.triggerPriority = ISR_PRIORITY_OS_TICK;  // ISR 우선순위
    stmCompareConfig.comparatorInterrupt = IfxStm_ComparatorInterrupt_ir0;
    stmCompareConfig.ticks = IFX_CFG_STM_TICKS_PER_MS * 10;   // 첫 인터럽트: 10ms 후
    stmCompareConfig.typeOfService = IfxSrc_Tos_cpu0;         // CPU0가 처리

    // STM 비교기 초기화
    IfxStm_initCompare(&MODULE_STM0, &stmCompareConfig);

    // ...
}
```

**IFX_CFG_STM_TICKS_PER_MS**:
```c
// Configuration.h
#define IFX_CFG_STM_TICKS_PER_MS    (100000)  // 100MHz / 1000 = 100,000 ticks/ms
```

### 8.3 STM ISR

```c
// Cpu0_Main.c

// ISR 선언
IFX_INTERRUPT(updateLwIPStackISR, 0, ISR_PRIORITY_OS_TICK);

// ISR 구현
void updateLwIPStackISR(void)
{
    // 1. 다음 인터럽트 예약 (1ms 후)
    IfxStm_increaseCompare(&MODULE_STM0, IfxStm_Comparator_0, IFX_CFG_STM_TICKS_PER_MS);

    // 2. 시스템 시간 증가
    g_TickCount_1ms++;

    // 3. lwIP 타이머 업데이트
    Ifx_Lwip_onTimerTick();
}
```

### 8.4 Ifx_Lwip_onTimerTick()

**역할**: 각 프로토콜 타이머 증가 및 플래그 설정

```c
// Ifx_Lwip.c
void Ifx_Lwip_onTimerTick(void)
{
    Ifx_Lwip *lwip = &g_Lwip;
    uint16 timerFlags = lwip->timerFlags;

    // ARP 타이머 증가
    lwip->timer.arp++;
    if (lwip->timer.arp >= IFX_LWIP_ARP_PERIOD)  // 1000ms
    {
        lwip->timer.arp = 0;
        timerFlags |= IFX_LWIP_FLAG_ARP;
    }

    // TCP Fast 타이머 증가
    lwip->timer.tcp_fast++;
    if (lwip->timer.tcp_fast >= IFX_LWIP_TCP_FAST_PERIOD)  // 250ms
    {
        lwip->timer.tcp_fast = 0;
        timerFlags |= IFX_LWIP_FLAG_TCP_FAST;
    }

    // TCP Slow 타이머 증가
    lwip->timer.tcp_slow++;
    if (lwip->timer.tcp_slow >= IFX_LWIP_TCP_SLOW_PERIOD)  // 500ms
    {
        lwip->timer.tcp_slow = 0;
        timerFlags |= IFX_LWIP_FLAG_TCP_SLOW;
    }

#if LWIP_DHCP
    // DHCP Coarse 타이머 증가
    lwip->timer.dhcp_coarse++;
    if (lwip->timer.dhcp_coarse >= IFX_LWIP_DHCP_COARSE_PERIOD)  // 60000ms
    {
        lwip->timer.dhcp_coarse = 0;
        timerFlags |= IFX_LWIP_FLAG_DHCP_COARSE;
    }

    // DHCP Fine 타이머 증가
    lwip->timer.dhcp_fine++;
    if (lwip->timer.dhcp_fine >= IFX_LWIP_DHCP_FINE_PERIOD)  // 500ms
    {
        lwip->timer.dhcp_fine = 0;
        timerFlags |= IFX_LWIP_FLAG_DHCP_FINE;
    }
#endif

    // Link 상태 확인 타이머 증가
    lwip->timer.link++;
    if (lwip->timer.link >= IFX_LWIP_LINK_PERIOD)  // 100ms
    {
        lwip->timer.link = 0;
        timerFlags |= IFX_LWIP_FLAG_LINK;
    }

    // 플래그 저장
    lwip->timerFlags = timerFlags;
}
```

### 8.5 Ifx_Lwip_pollTimerFlags()

**역할**: 타이머 플래그 확인 및 프로토콜 실행

```c
// Ifx_Lwip.c
void Ifx_Lwip_pollTimerFlags(void)
{
    Ifx_Lwip *lwip = &g_Lwip;
    uint16 timerFlags = lwip->timerFlags;

    // ARP 타이머
    if (timerFlags & IFX_LWIP_FLAG_ARP)
    {
        etharp_tmr();
        timerFlags &= ~IFX_LWIP_FLAG_ARP;
    }

    // TCP Fast 타이머
    if (timerFlags & IFX_LWIP_FLAG_TCP_FAST)
    {
        tcp_fasttmr();
        timerFlags &= ~IFX_LWIP_FLAG_TCP_FAST;
    }

    // TCP Slow 타이머
    if (timerFlags & IFX_LWIP_FLAG_TCP_SLOW)
    {
        tcp_slowtmr();
        timerFlags &= ~IFX_LWIP_FLAG_TCP_SLOW;
    }

#if LWIP_DHCP
    // DHCP Coarse 타이머
    if (timerFlags & IFX_LWIP_FLAG_DHCP_COARSE)
    {
        dhcp_coarse_tmr();
        timerFlags &= ~IFX_LWIP_FLAG_DHCP_COARSE;
    }

    // DHCP Fine 타이머
    if (timerFlags & IFX_LWIP_FLAG_DHCP_FINE)
    {
        dhcp_fine_tmr();
        timerFlags &= ~IFX_LWIP_FLAG_DHCP_FINE;
    }
#endif

    // Link 상태 확인
    if (timerFlags & IFX_LWIP_FLAG_LINK)
    {
        // PHY Link 상태 확인
        uint32 linkStatus = IfxGeth_Eth_Phy_Dp83825i_link_status();
        
        if (linkStatus)
        {
            netif_set_link_up(&lwip->netif);
        }
        else
        {
            netif_set_link_down(&lwip->netif);
        }
        
        timerFlags &= ~IFX_LWIP_FLAG_LINK;
    }

    // 플래그 저장
    lwip->timerFlags = timerFlags;
}
```

---

## 9. Echo 서버 구현 (TCP RAW API)

### 9.1 Echo 서버 개요

**기능**: 
- TCP 포트 80에서 클라이언트 연결 대기
- 클라이언트가 보낸 텍스트를 그대로 다시 전송 (Echo)
- Infineon 로고 출력

**구현 방식**: lwIP RAW API (콜백 기반)

### 9.2 Echo 세션 상태

```c
// Echo.c
enum EchoStates
{
    ES_NONE = 0,        // 초기화 안됨
    ES_ACCEPTED,        // 클라이언트 연결됨, 리소스 할당 중
    ES_RECEIVING,       // 데이터 수신 중
    ES_CLOSING          // 연결 종료 중, 리소스 해제 예정
};

typedef struct
{
    u8_t state;                         // 현재 상태
    struct tcp_pcb *pcb;                // TCP 제어 블록
    struct pbuf *p;                     // 수신 패킷 버퍼 체인
    char storage[STORAGE_SIZE_BYTES];   // 처리된 데이터 저장소 (256 bytes)
    uint16 nextFreeStoragePos;          // 저장소 내 다음 빈 위치
} EchoSession;
```

### 9.3 Echo 초기화

```c
// Echo.c
void echoInit(void)
{
    // 1. TCP 제어 블록 생성
    g_echoPcb = tcp_new();
    
    if (g_echoPcb != NULL)
    {
        // 2. 포트 80에 바인딩
        err_t err = tcp_bind(g_echoPcb, IP_ADDR_ANY, 80);
        
        if (err == ERR_OK)
        {
            // 3. Listen 모드로 전환
            g_echoPcb = tcp_listen(g_echoPcb);
            
            // 4. Accept 콜백 등록
            tcp_accept(g_echoPcb, echoAccept);
        }
        else
        {
            LWIP_DEBUGF(ECHO_DEBUG, ("Echo: unable to bind to port 80.\n"));
        }
    }
    else
    {
        LWIP_DEBUGF(ECHO_DEBUG, ("Echo: unable to create TCP control block.\n"));
    }
}
```

### 9.4 콜백 함수 체인

```
Client 연결 요청
    |
    v
echoAccept()                  ← 새 클라이언트 연결 시
    |
    +---> EchoSession 할당
    +---> 콜백 등록:
    |       - tcp_recv(newPcb, echoRecv)
    |       - tcp_sent(newPcb, echoSent)
    |       - tcp_err(newPcb, echoError)
    |       - tcp_poll(newPcb, echoPoll)
    +---> Infineon 로고 전송
    |
    v
데이터 수신
    |
    v
echoRecv()                    ← 데이터 수신 시
    |
    +---> echoUnpack()        ← pbuf → storage 복사
    +---> echoSend()          ← storage → 클라이언트 전송
    |
    v
데이터 전송 완료
    |
    v
echoSent()                    ← 전송 완료 시
    |
    +---> 추가 데이터 있으면 전송
    |
    v
주기적 체크
    |
    v
echoPoll()                    ← 500ms마다 (TCP Slow)
    |
    +---> 미처리 데이터 확인
    +---> ES_CLOSING 상태면 종료
    |
    v
에러 발생
    |
    v
echoError()                   ← 치명적 에러 시
    |
    +---> EchoSession 해제
```

### 9.5 echoAccept() - 연결 수락

```c
// Echo.c
err_t echoAccept(void *arg, struct tcp_pcb *newPcb, err_t err)
{
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(err);

    // 1. EchoSession 메모리 할당
    EchoSession *es = (EchoSession *)mem_malloc(sizeof(EchoSession));
    
    if (es != NULL)
    {
        // 2. 세션 초기화
        es->state = ES_ACCEPTED;
        es->pcb = newPcb;
        es->p = NULL;
        memset(es->storage, 0, STORAGE_SIZE_BYTES);
        es->nextFreeStoragePos = 0;

        // 3. 콜백 등록
        tcp_arg(newPcb, es);               // 세션을 arg로 전달
        tcp_recv(newPcb, echoRecv);        // 수신 콜백
        tcp_sent(newPcb, echoSent);        // 전송 완료 콜백
        tcp_err(newPcb, echoError);        // 에러 콜백
        tcp_poll(newPcb, echoPoll, 0);     // 폴링 콜백 (500ms마다)

        // 4. Infineon 로고 전송
        tcp_write(newPcb, g_Logo, strlen(g_Logo), 1);

        return ERR_OK;
    }
    else
    {
        return ERR_MEM;  // 메모리 부족
    }
}
```

### 9.6 echoRecv() - 데이터 수신

```c
// Echo.c
err_t echoRecv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    EchoSession *es = (EchoSession *)arg;

    // Case 1: 원격 클라이언트가 연결 종료 (p == NULL)
    if (p == NULL)
    {
        es->state = ES_CLOSING;
        
        if (es->p == NULL)
        {
            echoClose(tpcb, es);  // 즉시 종료
        }
        else
        {
            echoUnpack(tpcb, es);  // 남은 데이터 처리
            echoSend(tpcb, es);
        }
        
        return ERR_OK;
    }

    // Case 2: 수신 에러
    if (err != ERR_OK)
    {
        if (p != NULL)
        {
            es->p = NULL;
            pbuf_free(p);
        }
        return err;
    }

    // Case 3: ES_ACCEPTED 상태 (첫 데이터)
    if (es->state == ES_ACCEPTED)
    {
        es->state = ES_RECEIVING;
        es->p = p;
        echoUnpack(tpcb, es);
        echoSend(tpcb, es);
        return ERR_OK;
    }

    // Case 4: ES_RECEIVING 상태 (추가 데이터)
    if (es->state == ES_RECEIVING)
    {
        if (es->p == NULL)
        {
            es->p = p;
            echoUnpack(tpcb, es);
            echoSend(tpcb, es);
        }
        else
        {
            // 기존 pbuf 체인에 새 pbuf 연결
            pbuf_chain(es->p, p);
        }
        return ERR_OK;
    }

    // Case 5: 알 수 없는 상태 (데이터 무시)
    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);
    return ERR_OK;
}
```

### 9.7 echoUnpack() - 데이터 언팩

```c
// Echo.c
void echoUnpack(struct tcp_pcb *tpcb, EchoSession *es)
{
    struct pbuf *ptr;

    while (es->p != NULL)
    {
        ptr = es->p;

        // 저장소에 공간이 있는지 확인
        if (es->nextFreeStoragePos + ptr->len <= STORAGE_SIZE_BYTES)
        {
            // pbuf에서 storage로 복사
            memcpy(&es->storage[es->nextFreeStoragePos], 
                   ptr->payload, 
                   ptr->len);
            es->nextFreeStoragePos += ptr->len;

            uint16 plen = ptr->len;

            // 다음 pbuf로 이동
            es->p = ptr->next;
            if (es->p != NULL)
            {
                pbuf_ref(es->p);  // 참조 카운트 증가
            }

            // 현재 pbuf 해제
            uint8 freed;
            do
            {
                freed = pbuf_free(ptr);
            } while (freed == 0);

            // lwIP에 수신 확인 전송
            tcp_recved(tpcb, plen);
        }
        else
        {
            break;  // 저장소 가득 참
        }
    }
}
```

### 9.8 echoSend() - 데이터 전송

```c
// Echo.c
void echoSend(struct tcp_pcb *tpcb, EchoSession *es)
{
    // 1. 저장소가 비어있으면 리턴
    if (es->nextFreeStoragePos == 0)
    {
        return;
    }

    // 2. 줄바꿈 문자로 끝나거나 저장소가 가득 찼을 때만 전송
    if (es->storage[es->nextFreeStoragePos - 1] != '\n' &&
        es->nextFreeStoragePos < STORAGE_SIZE_BYTES)
    {
        return;  // 아직 전송하지 않음
    }

    // 3. Echo 프리앰블 + 데이터 전송
    err_t wrErr = tcp_write(tpcb, "Board: ", 7, 1);  // TCP_WRITE_FLAG_COPY
    wrErr |= tcp_write(tpcb, es->storage, es->nextFreeStoragePos, 1);

    if (wrErr == ERR_OK)
    {
        es->nextFreeStoragePos = 0;  // 저장소 비우기
    }
}
```

### 9.9 echoClose() - 연결 종료

```c
// Echo.c
void echoClose(struct tcp_pcb *tpcb, EchoSession *es)
{
    // 1. 콜백 해제
    tcp_arg(tpcb, NULL);
    tcp_sent(tpcb, NULL);
    tcp_recv(tpcb, NULL);
    tcp_err(tpcb, NULL);
    tcp_poll(tpcb, NULL, 0);

    // 2. 세션 메모리 해제
    if (es != NULL)
    {
        mem_free(es);
    }

    // 3. TCP 연결 종료
    tcp_close(tpcb);
}
```

---

## 10. DHCP 구성

### 10.1 DHCP 개요

**DHCP** (Dynamic Host Configuration Protocol): 동적 IP 주소 할당 프로토콜

**역할**:
- IP 주소 자동 할당
- Netmask, Gateway, DNS 서버 정보 제공
- Hostname 등록 (Option 12)

### 10.2 DHCP 활성화

```c
// lwipopts.h
#define LWIP_DHCP               1       // DHCP 클라이언트 활성화
#define LWIP_NETIF_HOSTNAME     1       // Hostname 전송 활성화
#define BOARDNAME               "AURIXLK2TC375TP"  // Hostname
```

### 10.3 DHCP 시작

```c
// Ifx_Lwip.c - Ifx_Lwip_init() 내부
void Ifx_Lwip_init(eth_addr_t ethAddr)
{
    // ...

    // netif 등록
    netif_add(&lwip->netif, IP_ADDR_ANY, IP_ADDR_ANY, IP_ADDR_ANY, 
              NULL, ifx_netif_init, ethernet_input);

    // ...

#if LWIP_DHCP
    // DHCP 시작
    dhcp_start(&lwip->netif);
    
    // Hostname 설정
    netif_set_hostname(&lwip->netif, BOARDNAME);
#endif

    // ...
}
```

### 10.4 DHCP 상태 확인

```c
// 애플리케이션 코드
void checkDHCPStatus(void)
{
    struct netif *netif = Ifx_Lwip_getNetIf();
    struct dhcp *dhcp = netif_dhcp_data(netif);

    if (dhcp != NULL)
    {
        switch (dhcp->state)
        {
            case DHCP_STATE_OFF:
                // DHCP 꺼짐
                break;

            case DHCP_STATE_REQUESTING:
                // DHCP 요청 중
                break;

            case DHCP_STATE_BOUND:
                // IP 주소 할당 완료
                uint8 *ipAddr = Ifx_Lwip_getIpAddrPtr();
                printf("IP: %d.%d.%d.%d\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
                break;

            // ... 기타 상태 ...
        }
    }
}
```

### 10.5 Static IP 설정 (DHCP 비활성화 시)

```c
// lwipopts.h
#define LWIP_DHCP               0       // DHCP 비활성화

// Ifx_Lwip.c - Ifx_Lwip_init() 수정
void Ifx_Lwip_init(eth_addr_t ethAddr)
{
    // ...

    // Static IP 설정
    ip_addr_t ipAddr, netmask, gateway;
    IP4_ADDR(&ipAddr, 192, 168, 1, 10);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gateway, 192, 168, 1, 1);

    // netif 등록
    netif_add(&lwip->netif, &ipAddr, &netmask, &gateway, 
              NULL, ifx_netif_init, ethernet_input);

    // ...
}
```

---

## 11. UART 디버깅

### 11.1 UART 로깅 설정

**목적**: lwIP 디버그 메시지를 UART로 출력

**파일**: `UART_Logging.c/h`

### 11.2 LWIP_PLATFORM_DIAG 재정의

```c
// Libraries/Ethernet/lwip/port/include/arch/cc.h

#include "UART_Logging.h"

// lwIP 디버그 매크로 재정의
#define LWIP_PLATFORM_DIAG(x)   do { UART_Logging x; } while(0)

// 예제:
// LWIP_DEBUGF(NETIF_DEBUG, ("IP address: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]));
// → UART_Logging("IP address: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
```

### 11.3 UART_Logging 구현 (간략)

```c
// UART_Logging.c
#include "IfxAsclin_Asc.h"
#include <stdarg.h>
#include <stdio.h>

// ASCLIN0 (UART) 구조체
static IfxAsclin_Asc g_ascHandle;

void UART_Logging_init(void)
{
    // ASCLIN0 초기화 (115200 baud, 8N1)
    IfxAsclin_Asc_Config ascConfig;
    IfxAsclin_Asc_initModuleConfig(&ascConfig, &MODULE_ASCLIN0);
    
    ascConfig.baudrate.baudrate = 115200;
    ascConfig.baudrate.oversampling = IfxAsclin_OversamplingFactor_16;
    
    // TX/RX 핀 설정 (보드마다 다름)
    ascConfig.pins.tx = &IfxAsclin0_TX_P14_0_OUT;
    ascConfig.pins.rx = &IfxAsclin0_RXA_P14_1_IN;
    
    IfxAsclin_Asc_initModule(&g_ascHandle, &ascConfig);
}

void UART_Logging(const char *format, ...)
{
    char buffer[256];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // UART로 전송
    IfxAsclin_Asc_write(&g_ascHandle, (uint8 *)buffer, NULL, strlen(buffer));
}
```

### 11.4 디버그 메시지 활성화

```c
// lwipopts.h

#define __LWIP_DEBUG__                  // 전체 디버깅 활성화
#define LWIP_DEBUG                      // lwIP 디버깅 활성화

// 개별 모듈 디버그 레벨
#define NETIF_DEBUG             LWIP_DBG_ON     // netif 디버그
#define DHCP_DEBUG              LWIP_DBG_OFF    // DHCP 디버그 끄기
#define TCP_DEBUG               LWIP_DBG_OFF    // TCP 디버그 끄기
#define ECHO_DEBUG              LWIP_DBG_ON     // Echo 디버그

// 디버그 타입 필터
#define LWIP_DBG_TYPES_ON       LWIP_DBG_STATE  // STATE 메시지만 출력
```

**디버그 타입**:
- `LWIP_DBG_TRACE`: 함수 호출 추적
- `LWIP_DBG_STATE`: 상태 변화
- `LWIP_DBG_FRESH`: 새 데이터 도착
- `LWIP_DBG_HALT`: 치명적 오류

---

## 12. 실전 코드 예제

### 12.1 완전한 메인 루프

```c
// Cpu0_Main.c
#include "Ifx_Types.h"
#include "IfxStm.h"
#include "IfxGeth_Eth.h"
#include "Ifx_Lwip.h"
#include "Echo.h"
#include "UART_Logging.h"

IFX_ALIGN(4) IfxCpu_syncEvent g_cpuSyncEvent = 0;

void core0_main(void)
{
    // 1. 인터럽트 활성화
    IfxCpu_enableInterrupts();

    // 2. Watchdog 비활성화
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    // 3. CPU 동기화
    IfxCpu_emitEvent(&g_cpuSyncEvent);
    IfxCpu_waitEvent(&g_cpuSyncEvent, 1);

    // 4. UART 디버깅 초기화
    UART_Logging_init();

    // 5. STM 타이머 초기화 (1ms 주기)
    IfxStm_CompareConfig stmCompareConfig;
    IfxStm_initCompareConfig(&stmCompareConfig);
    stmCompareConfig.triggerPriority = ISR_PRIORITY_OS_TICK;
    stmCompareConfig.comparatorInterrupt = IfxStm_ComparatorInterrupt_ir0;
    stmCompareConfig.ticks = IFX_CFG_STM_TICKS_PER_MS * 10;  // 첫 인터럽트: 10ms
    stmCompareConfig.typeOfService = IfxSrc_Tos_cpu0;
    IfxStm_initCompare(&MODULE_STM0, &stmCompareConfig);

    // 6. GETH 모듈 활성화
    IfxGeth_enableModule(&MODULE_GETH);

    // 7. MAC 주소 정의
    eth_addr_t ethAddr;
    ethAddr.addr[0] = 0xDE;
    ethAddr.addr[1] = 0xAD;
    ethAddr.addr[2] = 0xBE;
    ethAddr.addr[3] = 0xEF;
    ethAddr.addr[4] = 0xFE;
    ethAddr.addr[5] = 0xED;

    // 8. lwIP 초기화
    Ifx_Lwip_init(ethAddr);

    // 9. Echo 서버 초기화
    echoInit();

    UART_Logging("Ethernet Echo Server Ready!\n");

    // 10. 메인 루프
    while (1)
    {
        // lwIP 타이머 폴링 (ARP, TCP, DHCP 등)
        Ifx_Lwip_pollTimerFlags();

        // Ethernet 패킷 수신 처리
        Ifx_Lwip_pollReceiveFlags();
    }
}

// STM ISR
IFX_INTERRUPT(updateLwIPStackISR, 0, ISR_PRIORITY_OS_TICK);

void updateLwIPStackISR(void)
{
    // 다음 인터럽트 예약 (1ms 후)
    IfxStm_increaseCompare(&MODULE_STM0, IfxStm_Comparator_0, IFX_CFG_STM_TICKS_PER_MS);

    // 시스템 시간 증가
    g_TickCount_1ms++;

    // lwIP 타이머 업데이트
    Ifx_Lwip_onTimerTick();
}
```

### 12.2 Zonal Gateway용 TCP 서버 (DoIP 예제)

```c
/******************************************************************************
 * File: doip_server.c
 * Description: DoIP Server for Zonal Gateway (TCP RAW API)
 ******************************************************************************/

#include "lwip/tcp.h"
#include <string.h>

#define DOIP_PORT       13400
#define DOIP_BUFFER_SIZE 512

// DoIP 세션 구조체
typedef struct
{
    struct tcp_pcb *pcb;
    uint8 rxBuffer[DOIP_BUFFER_SIZE];
    uint16 rxLen;
} DoIPSession;

// 전역 변수
static struct tcp_pcb *g_doipListenPcb;

// 콜백 함수 프로토타입
static err_t doip_accept(void *arg, struct tcp_pcb *newPcb, err_t err);
static err_t doip_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void  doip_error(void *arg, err_t err);

// DoIP 서버 초기화
void doip_server_init(void)
{
    g_doipListenPcb = tcp_new();
    
    if (g_doipListenPcb != NULL)
    {
        err_t err = tcp_bind(g_doipListenPcb, IP_ADDR_ANY, DOIP_PORT);
        
        if (err == ERR_OK)
        {
            g_doipListenPcb = tcp_listen(g_doipListenPcb);
            tcp_accept(g_doipListenPcb, doip_accept);
            
            UART_Logging("DoIP Server listening on port %d\n", DOIP_PORT);
        }
    }
}

// Accept 콜백
static err_t doip_accept(void *arg, struct tcp_pcb *newPcb, err_t err)
{
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(err);
    
    // DoIP 세션 할당
    DoIPSession *session = (DoIPSession *)mem_malloc(sizeof(DoIPSession));
    
    if (session != NULL)
    {
        session->pcb = newPcb;
        session->rxLen = 0;
        memset(session->rxBuffer, 0, DOIP_BUFFER_SIZE);
        
        // 콜백 등록
        tcp_arg(newPcb, session);
        tcp_recv(newPcb, doip_recv);
        tcp_err(newPcb, doip_error);
        
        UART_Logging("DoIP: New client connected\n");
        
        return ERR_OK;
    }
    else
    {
        return ERR_MEM;
    }
}

// Receive 콜백
static err_t doip_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    DoIPSession *session = (DoIPSession *)arg;
    
    // 연결 종료
    if (p == NULL)
    {
        tcp_close(tpcb);
        if (session != NULL)
        {
            mem_free(session);
        }
        UART_Logging("DoIP: Client disconnected\n");
        return ERR_OK;
    }
    
    // 데이터 복사
    if (session->rxLen + p->tot_len <= DOIP_BUFFER_SIZE)
    {
        pbuf_copy_partial(p, &session->rxBuffer[session->rxLen], p->tot_len, 0);
        session->rxLen += p->tot_len;
        
        // DoIP 메시지 처리 (여기서는 간단히 로깅만)
        UART_Logging("DoIP: Received %d bytes\n", p->tot_len);
        
        // TODO: DoIP 메시지 파싱 및 처리
        // - Generic Header 확인
        // - Payload Type 확인 (Diagnostic Message, Routing Activation 등)
        // - UDS 메시지 추출 및 ECU로 라우팅
        
        tcp_recved(tpcb, p->tot_len);
    }
    
    pbuf_free(p);
    return ERR_OK;
}

// Error 콜백
static void doip_error(void *arg, err_t err)
{
    DoIPSession *session = (DoIPSession *)arg;
    
    if (session != NULL)
    {
        mem_free(session);
    }
    
    UART_Logging("DoIP: Connection error (%d)\n", err);
}
```

### 12.3 UDP 예제 (Heartbeat 전송)

```c
/******************************************************************************
 * File: heartbeat.c
 * Description: Periodic heartbeat to VMG via UDP
 ******************************************************************************/

#include "lwip/udp.h"
#include "lwip/ip_addr.h"

#define VMG_IP_ADDR     IP4_ADDR(192, 168, 1, 100)  // VMG IP
#define HEARTBEAT_PORT  5000
#define HEARTBEAT_INTERVAL_MS  1000  // 1초

static struct udp_pcb *g_heartbeatPcb;
static ip_addr_t g_vmgIpAddr;
static uint32 g_lastHeartbeatTime = 0;

// Heartbeat 초기화
void heartbeat_init(void)
{
    g_heartbeatPcb = udp_new();
    
    if (g_heartbeatPcb != NULL)
    {
        // VMG IP 주소 설정
        IP4_ADDR(&g_vmgIpAddr, 192, 168, 1, 100);
        
        UART_Logging("Heartbeat: Initialized (VMG: 192.168.1.100:%d)\n", HEARTBEAT_PORT);
    }
}

// Heartbeat 전송 (메인 루프에서 호출)
void heartbeat_poll(void)
{
    if (g_heartbeatPcb == NULL)
        return;
    
    // 1초마다 전송
    uint32 currentTime = g_TickCount_1ms;
    if (currentTime - g_lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS)
    {
        g_lastHeartbeatTime = currentTime;
        
        // Heartbeat 메시지 생성
        const char *msg = "HEARTBEAT:ZONAL_GATEWAY:OK";
        uint16 msgLen = strlen(msg);
        
        // pbuf 할당
        struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, msgLen, PBUF_RAM);
        
        if (p != NULL)
        {
            // 데이터 복사
            memcpy(p->payload, msg, msgLen);
            
            // UDP 전송
            err_t err = udp_sendto(g_heartbeatPcb, p, &g_vmgIpAddr, HEARTBEAT_PORT);
            
            if (err == ERR_OK)
            {
                UART_Logging("Heartbeat: Sent to VMG\n");
            }
            
            // pbuf 해제
            pbuf_free(p);
        }
    }
}
```

---

## 13. 요약 및 체크리스트

### 13.1 Ethernet 초기화 체크리스트

- [ ] **GETH 모듈 활성화** (`IfxGeth_enableModule`)
- [ ] **MAC 주소 정의** (Locally Administered Address)
- [ ] **lwIP 초기화** (`Ifx_Lwip_init`)
- [ ] **PHY 초기화** (DP83825I)
- [ ] **STM 타이머 설정** (1ms 주기 ISR)
- [ ] **netif 등록 및 활성화**
- [ ] **DHCP 시작** (또는 Static IP 설정)
- [ ] **메인 루프**: `Ifx_Lwip_pollTimerFlags()` + `Ifx_Lwip_pollReceiveFlags()`

### 13.2 lwIP 설정 체크리스트

- [ ] **NO_SYS = 1** (Bare-metal 모드)
- [ ] **LWIP_NETCONN = 0**, **LWIP_SOCKET = 0** (RAW API 사용)
- [ ] **MEM_SIZE** 충분한 Heap 크기 (최소 25KB)
- [ ] **LWIP_DHCP = 1** (DHCP 사용 시)
- [ ] **ETH_PAD_SIZE = 2** (Payload 정렬)
- [ ] **LWIP_DEBUG** 활성화 (디버깅 필요 시)

### 13.3 TCP RAW API 사용 체크리스트

- [ ] **tcp_new()**: TCP 제어 블록 생성
- [ ] **tcp_bind()**: IP/포트 바인딩
- [ ] **tcp_listen()**: Listen 모드 전환
- [ ] **tcp_accept()**: Accept 콜백 등록
- [ ] **tcp_recv()**: Receive 콜백 등록
- [ ] **tcp_sent()**: Sent 콜백 등록
- [ ] **tcp_err()**: Error 콜백 등록
- [ ] **tcp_poll()**: Poll 콜백 등록
- [ ] **tcp_write()**: 데이터 전송
- [ ] **tcp_recved()**: 수신 확인 (Window Update)
- [ ] **tcp_close()**: 연결 종료

### 13.4 일반적인 함정

1. **pbuf 메모리 누수**:
   - `pbuf_free()` 호출 잊지 말 것
   - `pbuf_ref()` 후 반드시 `pbuf_free()` 호출

2. **tcp_recved() 누락**:
   - 수신한 데이터 길이만큼 `tcp_recved()` 호출 필수
   - 누락 시 Window가 닫혀 더 이상 데이터 수신 불가

3. **tcp_write() 플래그**:
   - `TCP_WRITE_FLAG_COPY (1)`: lwIP가 데이터를 복사 (권장)
   - `TCP_WRITE_FLAG_MORE (2)`: Nagle 알고리즘 제어

4. **콜백에서 tcp_close() 호출 주의**:
   - `tcp_recv()`, `tcp_err()` 콜백에서 `tcp_close()` 호출 시 주의
   - 콜백 종료 후 `pcb`가 유효하지 않을 수 있음

5. **Interrupt Context에서 lwIP 호출 금지**:
   - lwIP는 Thread-safe 아님 (NO_SYS=1)
   - ISR에서는 플래그만 설정하고, 메인 루프에서 처리

### 13.5 성능 최적화 팁

1. **DMA 버퍼 크기**:
   - `IFXGETH_MAX_TX_BUFFER_SIZE`, `IFXGETH_MAX_RX_BUFFER_SIZE` 증가

2. **pbuf Pool 크기**:
   - `PBUF_POOL_SIZE` 증가 (lwipopts.h)

3. **TCP Window 크기**:
   - `TCP_WND` 증가 (기본값: 4KB)

4. **Checksum Offload**:
   - GETH 하드웨어 Checksum 기능 활성화

---

## 14. 참고 자료

- **lwIP Documentation**: <https://www.nongnu.org/lwip/>
- **AURIX iLLD User Manual**: `TC37A_iLLD_UM_1_0_1_17_0.chm`
- **DP83825I Datasheet**: Texas Instruments
- **IEEE 802.3**: Ethernet Standard
- **RFC 2131**: DHCP Protocol
- **RFC 793**: TCP Protocol

---

**문서 버전**: 1.0  
**작성일**: 2025-11-02  
**대상 프로젝트**: Zonal Gateway Ethernet 통신

