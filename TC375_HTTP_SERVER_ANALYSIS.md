# TC375 HTTP Server 구현 분석 (iLLD_TC3XX_ADS_GETH_LWIP_HTTP)

> 출처: `iLLD_TC3XX_ADS_GETH_LWIP_HTTP`  
> 목적: Zonal Gateway HTTP/REST API 서버 구현 참고

---

## 📋 목차

1. [개요](#1-개요)
2. [아키텍처](#2-아키텍처)
3. [HTTP 서버 (httpd)](#3-http-서버-httpd)
4. [SSI (Server Side Includes)](#4-ssi-server-side-includes)
5. [CGI (Common Gateway Interface)](#5-cgi-common-gateway-interface)
6. [웹페이지 생성 (fsdata)](#6-웹페이지-생성-fsdata)
7. [실전 코드 분석](#7-실전-코드-분석)
8. [Zonal Gateway 적용 가이드](#8-zonal-gateway-적용-가이드)

---

## 1. 개요

### 1.1 예제 목적

- **HTTP 서버**: 웹 브라우저를 통해 TC375 제어 및 모니터링
- **SSI**: 온도 센서 데이터를 실시간으로 웹 페이지에 전송
- **CGI**: LED ON/OFF 제어 (웹 페이지에서 버튼 클릭)

### 1.2 주요 특징

| 항목 | 설정 |
|------|------|
| **IP 주소** | Static IP: `192.168.2.20` (DHCP 비활성화) |
| **lwIP 모드** | Bare-metal (`NO_SYS=1`) |
| **HTTP 서버** | lwIP의 `httpd` 앱 |
| **웹페이지** | HTML/CSS/JS → C 배열 (fsdata.h) |
| **동적 데이터** | SSI (Server Side Includes) |
| **명령 처리** | CGI (Common Gateway Interface) |

---

## 2. 아키텍처

### 2.1 전체 구조

```
┌─────────────────────────────────────────────────────────────┐
│                    Web Browser (Client)                     │
│  - 192.168.2.20 접속                                         │
│  - index.htm 요청 및 렌더링                                  │
│  - JavaScript: 주기적 SSI 요청 (온도 데이터)                 │
│  - Button Click: CGI 요청 (LED 제어)                        │
└────────────────────┬────────────────────────────────────────┘
                     │ HTTP (TCP/IP)
┌────────────────────▼────────────────────────────────────────┐
│                  TC375 HTTP Server                          │
│  ┌────────────────────────────────────────────────────┐     │
│  │         lwIP HTTP Daemon (httpd)                   │     │
│  │  - Port 80 Listen                                  │     │
│  │  - fsdata.h 웹페이지 서빙                          │     │
│  │  - SSI Tag 처리 (<!--#cpu_temp-->)                 │     │
│  │  - CGI 요청 라우팅 (/ledcontrol.cgi)               │     │
│  └────────────────┬───────────────────┬─────────────────┘     │
│                   │                   │                       │
│        ┌──────────▼────────┐  ┌───────▼───────────┐         │
│        │  SSI Handler      │  │   CGI Handler     │         │
│        │  - g_cpu_temperature │  │  - LED Control │         │
│        │  - int_to_str()   │  │  - IfxPort_setPinXXX │   │
│        └───────────────────┘  └───────────────────┘         │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │         lwIP TCP/IP Stack                            │   │
│  │  - TCP, IP, ARP                                      │   │
│  │  - Bare-metal (NO_SYS=1)                             │   │
│  └──────────────────┬───────────────────────────────────┘   │
│                     │                                        │
│  ┌──────────────────▼───────────────────────────────────┐   │
│  │         GETH (Gigabit Ethernet MAC)                  │   │
│  │  - PHY: DP83825I (RMII)                              │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 데이터 흐름

#### 2.2.1 온도 데이터 조회 (SSI)

```
Browser                      TC375
   |                           |
   | GET /data.ssi             |
   |-------------------------->|
   |                           | CGI: data_handler() → "/data.ssi"
   |                           | httpd가 data.ssi 파싱
   |                           | <!--#cpu_temp--> 발견
   |                           | SSI: ssi_handler() 호출
   |                           | g_cpu_temperature → "25"
   |                           |
   | HTTP 200 OK               |
   | Body: "25"                |
   |<--------------------------|
   |                           |
```

#### 2.2.2 LED 제어 (CGI)

```
Browser                      TC375
   |                           |
   | GET /ledcontrol.cgi?led=led1
   |-------------------------->|
   |                           | CGI: ledcontrol_handler()
   |                           | pcValue[0] == "led1"
   |                           | IfxPort_setPinHigh(P00_5)
   |                           | return "/cgi.htm"
   |                           |
   | HTTP 302 Redirect         |
   | Location: /cgi.htm        |
   |<--------------------------|
   |                           |
   | GET /cgi.htm              |
   |-------------------------->|
   |                           | fsdata에서 cgi.htm 전송
   | HTTP 200 OK               |
   | Body: cgi.htm             |
   |<--------------------------|
```

---

## 3. HTTP 서버 (httpd)

### 3.1 httpd 초기화

```c
// Cpu0_Main.c
void core0_main(void)
{
    // ...
    Ifx_Lwip_init(eth_adr);   // lwIP 초기화
    
    httpd_init();             // HTTP 서버 초기화
    cgi_init();               // CGI 핸들러 등록
    ssi_init();               // SSI 핸들러 등록
    
    while (1)
    {
        Ifx_Lwip_pollTimerFlags();    // lwIP 타이머 폴링
        Ifx_Lwip_pollReceiveFlags();  // Ethernet 수신 폴링
    }
}
```

**`httpd_init()` 역할**:
- TCP 포트 80에서 Listen
- 새 연결 수락 및 HTTP 요청 파싱
- fsdata.h에서 파일 검색 및 전송
- SSI 태그 처리 및 CGI 라우팅

### 3.2 lwipopts.h 설정

```c
// Configurations/lwipopts.h

// HTTP 서버 활성화
#define LWIP_HTTPD_SSI              1   // SSI 지원
#define LWIP_HTTPD_CGI              1   // CGI 지원
#define LWIP_HTTPD_MAX_TAG_NAME_LEN 20  // SSI 태그 최대 길이
#define LWIP_HTTPD_SSI_INCLUDE_TAG  0   // <!--#tag--> 형식 사용
#define HTTPD_FSDATA_FILE           "fsdata.h"  // 웹페이지 데이터 파일

// 기타 설정
#define NO_SYS                      1   // Bare-metal
#define LWIP_DHCP                   0   // Static IP 사용
#define MEM_SIZE                    (25 * 1024)  // Heap 크기
```

---

## 4. SSI (Server Side Includes)

### 4.1 SSI 개념

**SSI**: HTML 파일에 특수 태그를 삽입하여 서버에서 동적 데이터로 치환

**형식**: `<!--#태그_이름-->`

**예제**:
```html
<!-- index.htm -->
<p>CPU Temperature: <!--#cpu_temp--> °C</p>
```

**서버 응답**:
```html
<p>CPU Temperature: 25 °C</p>
```

### 4.2 SSI 구현

#### 4.2.1 SSI 태그 정의

```c
// Httpd_SSI_CGI.c

// SSI 태그 배열
static const char *g_ssi_tags[] =
{
    "cpu_temp",  // <!--#cpu_temp--> 태그
};

// 전역 변수 (ISR에서 주기적 업데이트)
uint16 g_cpu_temperature = 20;  // 온도 값
```

#### 4.2.2 SSI 핸들러

```c
// Httpd_SSI_CGI.c

// SSI 핸들러: 태그를 데이터로 치환
static uint16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen)
{
    (void)iInsertLen;
    uint16 n;
    
    // iIndex: g_ssi_tags 배열의 인덱스
    // iIndex == 0 → "cpu_temp"
    
    // g_cpu_temperature를 문자열로 변환
    n = (uint16)int_to_str(pcInsert, g_cpu_temperature);
    
    return n;  // 변환된 문자열 길이 반환
}
```

#### 4.2.3 SSI 초기화

```c
// Httpd_SSI_CGI.c

void ssi_init(void)
{
    // SSI 핸들러 등록
    http_set_ssi_handler(ssi_handler,           // 핸들러 함수
                         g_ssi_tags,             // 태그 배열
                         LWIP_ARRAYSIZE(g_ssi_tags));  // 태그 개수
}
```

### 4.3 SSI 사용 예제

#### HTML 파일 (Apps/http/fs/data.ssi)
```html
<!--#cpu_temp-->
```

#### JavaScript (index.htm)
```javascript
// 1초마다 온도 데이터 요청
setInterval(function() {
    $.get("/data.cgi", function(data) {
        // data: SSI로 치환된 온도 값 (예: "25")
        $("#temperature").text(data);
    });
}, 1000);
```

---

## 5. CGI (Common Gateway Interface)

### 5.1 CGI 개념

**CGI**: 클라이언트 요청을 서버에서 처리하고 응답 페이지 지정

**형식**: `/파일명.cgi?param1=value1&param2=value2`

**예제**:
```
GET /ledcontrol.cgi?led=led1
```

### 5.2 CGI 구현

#### 5.2.1 CGI 핸들러 정의

```c
// Httpd_SSI_CGI.c

// CGI 핸들러 구조체 배열
tCGI led_handler_struct[] =
{
    {
        .pcCGIName = "/ledcontrol.cgi",    // CGI 경로
        .pfnCGIHandler = ledcontrol_handler  // 핸들러 함수
    },
    {
        .pcCGIName = "/data.cgi",          // 온도 조회
        .pfnCGIHandler = data_handler
    }
};
```

#### 5.2.2 LED 제어 CGI 핸들러

```c
// Httpd_SSI_CGI.c

// CGI 핸들러: LED 제어
const char *ledcontrol_handler(int iIndex, int iNumParams, 
                                 char *pcParam[], char *pcValue[])
{
    (void)iIndex;
    (void)iNumParams;
    (void)pcParam;
    
    // GET /ledcontrol.cgi?led=led0 → LED OFF
    if (strcmp(pcValue[0], "led0") == 0)
    {
        IfxPort_setPinLow(IfxPort_P00_5.port, IfxPort_P00_5.pinIndex);
    }
    // GET /ledcontrol.cgi?led=led1 → LED ON
    else if (strcmp(pcValue[0], "led1") == 0)
    {
        IfxPort_setPinHigh(IfxPort_P00_5.port, IfxPort_P00_5.pinIndex);
    }
    
    // 응답 페이지 지정 (리다이렉트)
    return "/cgi.htm";
}
```

#### 5.2.3 데이터 조회 CGI 핸들러

```c
// Httpd_SSI_CGI.c

// CGI 핸들러: 온도 조회 (SSI 파일 반환)
const char *data_handler(int iIndex, int iNumParams, 
                          char *pcParam[], char *pcValue[])
{
    (void)iIndex;
    (void)iNumParams;
    (void)pcParam;
    (void)pcValue;
    
    // SSI 파일 반환 (httpd가 SSI 태그 처리)
    return "/data.ssi";
}
```

#### 5.2.4 CGI 초기화

```c
// Httpd_SSI_CGI.c

int cgi_init(void)
{
    // CGI 핸들러 등록
    http_set_cgi_handlers(led_handler_struct,  // 핸들러 배열
                          2);                   // 핸들러 개수
    
    // LED 핀 초기화
    IfxPort_setPinModeOutput(IfxPort_P00_5.port, IfxPort_P00_5.pinIndex, 
                             IfxPort_OutputMode_pushPull, 
                             IfxPort_OutputIdx_general);
    IfxPort_setPinHigh(IfxPort_P00_5.port, IfxPort_P00_5.pinIndex);
    
    return 0;
}
```

### 5.3 CGI 사용 예제

#### HTML 파일 (Apps/http/fs/cgi.htm)
```html
<h2>LED Control</h2>
<a href="/ledcontrol.cgi?led=led1">
    <button>LED ON</button>
</a>
<a href="/ledcontrol.cgi?led=led0">
    <button>LED OFF</button>
</a>
```

---

## 6. 웹페이지 생성 (fsdata)

### 6.1 fsdata 개념

lwIP의 httpd는 **파일 시스템이 없는 환경**에서 동작하므로, 웹페이지를 **C 배열로 변환**하여 Flash에 저장합니다.

**변환 과정**:
```
HTML/CSS/JS 파일  →  makefsdata.exe  →  fsdata.h (C 배열)
```

### 6.2 웹페이지 구조

```
Apps/http/fs/
├── 404.html          # 404 에러 페이지
├── cgi.htm           # CGI 제어 페이지
├── data.ssi          # SSI 데이터 페이지 (온도)
├── favicon.ico       # 파비콘
├── img_sics.gif      # 이미지
├── index.htm         # 메인 페이지
├── jquery.min.js     # jQuery 라이브러리
└── smoothie.min.js   # 차트 라이브러리
```

### 6.3 fsdata 생성 과정

#### Step 1: HTML 파일 작성

```html
<!-- Apps/http/fs/index.htm -->
<!DOCTYPE html>
<html>
<head>
    <title>TC375 Web Server</title>
    <script src="/jquery.min.js"></script>
</head>
<body>
    <h1>AURIX TC375 Lite Kit</h1>
    <p>CPU Temperature: <span id="temperature">--</span> °C</p>
    
    <script>
        // 1초마다 온도 조회
        setInterval(function() {
            $.get("/data.cgi", function(data) {
                $("#temperature").text(data);
            });
        }, 1000);
    </script>
</body>
</html>
```

#### Step 2: makefsdata 실행

```cmd
# Apps/http/makefsdata/ 폴더에서 실행
cd Apps\http\makefsdata
makefsdata.exe
```

**makefsdata.exe 기능**:
- `fs/` 폴더의 모든 파일 읽기
- 각 파일을 C 배열로 변환
- `fsdata.h` 생성

#### Step 3: fsdata.h 생성 결과

```c
// Apps/http/fsdata.h (자동 생성)

// index.htm 데이터
static const unsigned char data_index_htm[] = {
    /* HTTP Header */
    0x48, 0x54, 0x54, 0x50, 0x2f, 0x31, 0x2e, 0x30, 0x20, 0x32, 0x30, 0x30,  // "HTTP/1.0 200 "
    0x20, 0x4f, 0x4b, 0x0d, 0x0a,  // "OK\r\n"
    // ... (헤더 계속) ...
    
    /* HTML Content */
    0x3c, 0x21, 0x44, 0x4f, 0x43, 0x54, 0x59, 0x50, 0x45,  // "<!DOCTYPE"
    // ... (HTML 내용) ...
};

// 파일 구조체
static const struct fsdata_file file_index_htm[] = {
    {
        file_404_html,          // 다음 파일 포인터
        data_index_htm,         // 파일 데이터
        data_index_htm + 12,    // 데이터 시작 (헤더 제외)
        sizeof(data_index_htm) - 12,  // 데이터 크기
        1                       // HTTP 헤더 포함 여부
    }
};
```

#### Step 4: 프로젝트에 반영

```c
// lwipopts.h
#define HTTPD_FSDATA_FILE "fsdata.h"  // fsdata 파일 지정
```

**httpd가 fsdata 사용 방법**:
1. 클라이언트가 `/index.htm` 요청
2. httpd가 `fsdata.h`에서 `file_index_htm` 검색
3. `data_index_htm` 배열 전송

### 6.4 fsdata 업데이트 워크플로우

1. **HTML/CSS/JS 수정** (`Apps/http/fs/`)
2. **makefsdata 실행** (`Apps/http/makefsdata/makefsdata.exe`)
3. **fsdata.h 복사** (`makefsdata/fsdata.h` → `Apps/http/fsdata.h`)
4. **Clean Build** (이클립스에서 Project → Clean)

---

## 7. 실전 코드 분석

### 7.1 메인 루프

```c
// Cpu0_Main.c
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
    
    // 4. STM 타이머 초기화 (1ms 주기)
    IfxStm_CompareConfig stmCompareConfig;
    IfxStm_initCompareConfig(&stmCompareConfig);
    stmCompareConfig.triggerPriority = ISR_PRIORITY_OS_TICK;
    stmCompareConfig.comparatorInterrupt = IfxStm_ComparatorInterrupt_ir0;
    stmCompareConfig.ticks = IFX_CFG_STM_TICKS_PER_MS * 10;
    stmCompareConfig.typeOfService = IfxSrc_Tos_cpu0;
    IfxStm_initCompare(&MODULE_STM0, &stmCompareConfig);
    
    // 5. GETH 모듈 활성화
    IfxGeth_enableModule(&MODULE_GETH);
    
    // 6. MAC 주소 정의
    eth_addr_t eth_adr;
    eth_adr.addr[0] = 0xDE;
    eth_adr.addr[1] = 0xAD;
    eth_adr.addr[2] = 0xBE;
    eth_adr.addr[3] = 0xEF;
    eth_adr.addr[4] = 0xFE;
    eth_adr.addr[5] = 0xED;
    
    // 7. lwIP 초기화
    Ifx_Lwip_init(eth_adr);
    
    // 8. HTTP 서버 초기화
    httpd_init();   // HTTP 서버
    cgi_init();     // CGI 핸들러
    ssi_init();     // SSI 핸들러
    
    // 9. 메인 루프
    while (1)
    {
        Ifx_Lwip_pollTimerFlags();    // lwIP 타이머 폴링
        Ifx_Lwip_pollReceiveFlags();  // Ethernet 수신 폴링
    }
}
```

### 7.2 STM ISR (1ms 주기)

```c
// Cpu0_Main.c

IFX_INTERRUPT(update_lwip_stack_isr, 0, ISR_PRIORITY_OS_TICK);

void update_lwip_stack_isr(void)
{
    // 1. 다음 인터럽트 예약 (1ms 후)
    IfxStm_increaseCompare(&MODULE_STM0, IfxStm_Comparator_0, IFX_CFG_STM_TICKS_PER_MS);
    
    // 2. lwIP 시스템 시간 증가
    g_TickCount_1ms++;
    
    // 3. lwIP 타이머 업데이트
    Ifx_Lwip_onTimerTick();
    
    // 4. 온도 센서 읽기
    dts_measurement();  // g_cpu_temperature 업데이트
}
```

### 7.3 Static IP 설정

```c
// Libraries/Ethernet/lwip/port/src/Ifx_Lwip.c

void Ifx_Lwip_init(eth_addr_t ethAddr)
{
    // Static IP 주소 설정
    ip_addr_t default_ipaddr, default_netmask, default_gw;
    
    IP4_ADDR(&default_ipaddr, 192, 168, 2, 20);   // IP: 192.168.2.20
    IP4_ADDR(&default_netmask, 255, 255, 255, 0); // Netmask: 255.255.255.0
    IP4_ADDR(&default_gw, 192, 168, 2, 1);        // Gateway: 192.168.2.1
    
    // lwIP 초기화
    lwip_init();
    
    // netif 등록 (Static IP 사용)
    g_Lwip.eth_addr = ethAddr;
    netif_add(&g_Lwip.netif, &default_ipaddr, &default_netmask, &default_gw,
              (void *)0, ifx_netif_init, ethernet_input);
    netif_set_default(&g_Lwip.netif);
    netif_set_up(&g_Lwip.netif);
    
    // DHCP는 시작하지 않음 (lwipopts.h에서 LWIP_DHCP = 0)
}
```

---

## 8. Zonal Gateway 적용 가이드

### 8.1 JSON API 서버 구현

#### 8.1.1 CGI 핸들러: JSON 응답

```c
// zonal_gateway_api.c

#include "lwip/apps/httpd.h"
#include <stdio.h>
#include <string.h>

// CGI 핸들러: VCI 조회
const char *vci_handler(int iIndex, int iNumParams, 
                         char *pcParam[], char *pcValue[])
{
    (void)iIndex;
    (void)iNumParams;
    (void)pcParam;
    (void)pcValue;
    
    // SSI 파일 반환 (JSON 형식)
    return "/vci.ssi";
}

// SSI 핸들러: VCI JSON 생성
static uint16_t api_ssi_handler(int iIndex, char *pcInsert, int iInsertLen)
{
    uint16 n = 0;
    
    switch (iIndex)
    {
        case 0:  // "vci_json"
            // JSON 문자열 생성
            n = snprintf(pcInsert, iInsertLen,
                "{\"zonal_id\":\"ZG01\",\"status\":\"OK\",\"ecu_count\":3}");
            break;
        
        case 1:  // "heartbeat_json"
            n = snprintf(pcInsert, iInsertLen,
                "{\"timestamp\":%u,\"alive\":true}", g_TickCount_1ms);
            break;
    }
    
    return n;
}

// SSI 태그 정의
static const char *g_api_ssi_tags[] =
{
    "vci_json",
    "heartbeat_json",
};

// CGI 핸들러 정의
tCGI api_handler_struct[] =
{
    { .pcCGIName = "/api/vci",       .pfnCGIHandler = vci_handler },
    { .pcCGIName = "/api/heartbeat", .pfnCGIHandler = heartbeat_handler },
};

void api_init(void)
{
    http_set_cgi_handlers(api_handler_struct, 2);
    http_set_ssi_handler(api_ssi_handler, g_api_ssi_tags, 2);
}
```

#### 8.1.2 SSI 파일: JSON 응답

```json
<!-- Apps/http/fs/vci.ssi -->
<!--#vci_json-->
```

#### 8.1.3 HTTP 헤더 설정 (JSON)

`makefsdata.exe`로 생성된 `fsdata.h`를 수정하여 Content-Type 설정:

```c
// fsdata.h (수동 수정 필요)

// vci.ssi 데이터
static const unsigned char data_vci_ssi[] = {
    /* HTTP Header */
    "HTTP/1.0 200 OK\r\n"
    "Server: lwIP/2.1.2\r\n"
    "Content-Type: application/json\r\n"  // JSON Content-Type
    "\r\n"
    
    /* JSON Content */
    "<!--#vci_json-->"
};
```

### 8.2 DoIP JSON API 서버

#### 8.2.1 API 엔드포인트

| 엔드포인트 | 메서드 | 설명 |
|-----------|--------|------|
| `/api/vci` | GET | Zone VCI 조회 |
| `/api/heartbeat` | GET | Heartbeat 조회 |
| `/api/ota/status` | GET | OTA 상태 조회 |
| `/api/ecu/list` | GET | ECU 목록 조회 |

#### 8.2.2 CGI vs TCP RAW API 비교

| 항목 | CGI (httpd) | TCP RAW API |
|------|-------------|-------------|
| **복잡도** | 낮음 (자동 처리) | 높음 (수동 구현) |
| **성능** | 낮음 (HTTP 오버헤드) | 높음 (TCP 직접 제어) |
| **유연성** | 제한적 (httpd 제약) | 높음 (완전 제어) |
| **디버깅** | 쉬움 (웹 브라우저) | 어려움 (전용 툴 필요) |
| **적합 사용처** | RESTful API, 모니터링 | DoIP, UDS, 고성능 통신 |

**권장**:
- **VMG ↔ Zonal Gateway (JSON API)**: TCP RAW API (Echo.c 참고)
- **사용자 모니터링 (Web UI)**: CGI + SSI (이 예제 참고)

### 8.3 실전 통합 예제

#### 8.3.1 Zonal Gateway 메인 함수

```c
// Cpu0_Main.c
void core0_main(void)
{
    // ... (GETH, lwIP 초기화) ...
    
    // HTTP 서버 (모니터링 용)
    httpd_init();
    api_init();        // JSON API (VCI, Heartbeat)
    
    // DoIP 서버 (포트 13400)
    doip_server_init();  // TCP RAW API
    
    // JSON 서버 (포트 8765)
    json_server_init();  // TCP RAW API
    
    while (1)
    {
        Ifx_Lwip_pollTimerFlags();
        Ifx_Lwip_pollReceiveFlags();
    }
}
```

---

## 📚 요약

### HTTP 서버 구현 체크리스트

- [ ] **httpd 초기화** (`httpd_init()`)
- [ ] **SSI 핸들러 구현** (동적 데이터)
- [ ] **CGI 핸들러 구현** (명령 처리)
- [ ] **웹페이지 작성** (HTML/CSS/JS)
- [ ] **fsdata 생성** (`makefsdata.exe`)
- [ ] **lwipopts.h 설정** (LWIP_HTTPD_SSI, LWIP_HTTPD_CGI)
- [ ] **Static IP 설정** (또는 DHCP)
- [ ] **메인 루프 구현** (폴링)

### Zonal Gateway 적용 포인트

1. **Web UI (모니터링)**:
   - httpd + SSI/CGI
   - VCI, Heartbeat, OTA Status 조회
   - LED/ECU 제어

2. **JSON API (VMG ↔ Zonal Gateway)**:
   - TCP RAW API 권장
   - Echo.c 예제 참고
   - 고성능, 낮은 레이턴시

3. **DoIP (VMG ↔ Zonal Gateway, Zonal Gateway ↔ ECU)**:
   - TCP RAW API 필수
   - 포트 13400
   - UDS 라우팅

---

**문서 버전**: 1.0  
**작성일**: 2025-11-02  
**대상 프로젝트**: Zonal Gateway HTTP Server 구현

