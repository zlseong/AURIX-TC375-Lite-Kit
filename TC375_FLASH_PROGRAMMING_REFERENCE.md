# TC375 Flash Programming - Complete Reference

> 출처: Infineon AURIX Code Examples - Flash_Programming_1_KIT_TC375_LK  
> 목적: Zonal Gateway 듀얼 뱅크 OTA 구현을 위한 완전한 참고 자료

---

## 📋 목차

1. [TC375 Flash 메모리 구조](#1-tc375-flash-메모리-구조)
2. [Flash 주소 맵](#2-flash-주소-맵)
3. [Flash API 사용법](#3-flash-api-사용법)
4. [PSPR (Program Scratch-Pad RAM) 사용](#4-pspr-program-scratch-pad-ram-사용)
5. [EndInit Protection](#5-endinit-protection)
6. [Linker Script 설정](#6-linker-script-설정)
7. [실전 코드 예제](#7-실전-코드-예제)
8. [듀얼 뱅크 OTA 구현 가이드](#8-듀얼-뱅크-ota-구현-가이드)

---

## 1. TC375 Flash 메모리 구조

### 1.1 Program Flash (PFLASH)

TC375는 **2개의 Program Flash Bank (PF0, PF1)** 를 가지고 있으며, 각 Bank는 **3MB** 크기입니다.

```
총 PFLASH 용량: 6MB (2 Banks × 3MB)

Bank 0 (PF0): 3MB
  - 3개의 Physical Sector (각 1MB)
  - 각 Physical Sector = 64개의 Logical Sector (각 16KB)
  - 총 192개의 Logical Sector

Bank 1 (PF1): 3MB
  - 동일한 구조 (192개의 Logical Sector)
```

**Physical Sector vs Logical Sector:**
- **Physical Sector**: 1MB (1024KB) 크기의 물리적 섹터 (3개/Bank)
- **Logical Sector**: 16KB 크기의 논리적 섹터 (192개/Bank)
- **Erase 작업**: Logical Sector 단위로만 가능
- **Page**: 32 Bytes (PFLASH의 최소 프로그래밍 단위)

### 1.2 Data Flash (DFLASH)

TC375는 **2개의 Data Flash Bank (DF0, DF1)** 를 가지고 있습니다.

```
DFLASH0: 256KB (EEPROM 에뮬레이션용)
  - 64개의 Logical Sector (각 4KB, single-ended mode)
  - UCB (User Configuration Block) 포함
  - CFS (Configuration Sector) 포함

DFLASH1: 128KB (EEPROM 에뮬레이션용)
  - 32개의 Logical Sector
```

**DFLASH 특성:**
- **Page**: 8 Bytes (DFLASH의 최소 프로그래밍 단위)
- **Mode**: Single-ended (4KB/sector) 또는 Complement sensing (2KB/sector)
- **사용 목적**: EEPROM 에뮬레이션, 설정 데이터 저장

---

## 2. Flash 주소 맵

### 2.1 PFLASH 주소 (iLLD 매크로 기준)

```c
// IfxFlash_cfg_TC37x.h에서 정의됨

// Cached 주소 (실행용)
#define IFXFLASH_PFLASH_START        0xA0000000    // PFLASH 전체 시작
#define IFXFLASH_PFLASH_SIZE         0x00600000    // 6MB

#define IFXFLASH_PFLASH_P0_START     0xA0000000    // Bank 0 시작
#define IFXFLASH_PFLASH_P0_SIZE      0x00300000    // 3MB
#define IFXFLASH_PFLASH_P0_END       0xA02FFFFF    // Bank 0 끝

#define IFXFLASH_PFLASH_P1_START     0xA0300000    // Bank 1 시작
#define IFXFLASH_PFLASH_P1_SIZE      0x00300000    // 3MB
#define IFXFLASH_PFLASH_P1_END       0xA05FFFFF    // Bank 1 끝

// Non-Cached 주소 (디버깅용)
#define IFXFLASH_PFLASH_P0_NC_START  0x80000000    // Bank 0 (캐시 우회)
#define IFXFLASH_PFLASH_P1_NC_START  0x80300000    // Bank 1 (캐시 우회)
```

**주소 변환 규칙:**
```
Cached Address    = 0xA0000000 + offset
Non-Cached Address = 0x80000000 + offset

예:
  Bank 0, offset 0x10000
    - Cached:     0xA0010000
    - Non-cached: 0x80010000
```

### 2.2 DFLASH 주소

```c
#define IFXFLASH_DFLASH_START        0xAF000000    // DFLASH0 시작
#define IFXFLASH_DFLASH_SIZE         0x00040000    // 256KB

// UCB (User Configuration Block)
#define IFXFLASH_UCB_START           0xAF400000    // 24KB
```

### 2.3 PSPR (Program Scratch-Pad RAM) 주소

```c
// CPU0 PSPR
#define PSPR0_START                  0x70100000    // 64KB
#define PSPR0_SIZE                   0x00010000

// CPU1 PSPR
#define PSPR1_START                  0x60100000    // 64KB
#define PSPR1_SIZE                   0x00010000

// CPU2 PSPR
#define PSPR2_START                  0x50100000    // 64KB
#define PSPR2_SIZE                   0x00010000
```

### 2.4 듀얼 뱅크 OTA를 위한 메모리 맵 제안

```c
/*
 * TC375 PFLASH Dual-Bank Memory Map for OTA
 * Total: 6MB (0x80000000 ~ 0x805FFFFF)
 */

// Bank 0 (PF0): 3MB
#define SSW_START                    0x80000000    // 16KB  (SSW - Startup Software)
#define SSW_SIZE                     0x00004000

#define BOOTLOADER_A_START           0x80004000    // 128KB (Bootloader Bank A)
#define BOOTLOADER_A_SIZE            0x00020000

#define APP_BANK_A_START             0x80024000    // ~2.85MB (Application Bank A)
#define APP_BANK_A_SIZE              0x002DC000    // 0x80300000 - 0x80024000

// Bank 1 (PF1): 3MB
#define BOOTLOADER_B_START           0x80300000    // 128KB (Bootloader Bank B)
#define BOOTLOADER_B_SIZE            0x00020000

#define APP_BANK_B_START             0x80320000    // ~2.87MB (Application Bank B)
#define APP_BANK_B_SIZE              0x002E0000    // 0x80600000 - 0x80320000
```

**주의사항:**
- SSW는 Bank 0의 맨 앞 16KB에 고정 (0x80000000)
- Bootloader A/B는 각 Bank에 128KB 할당
- Application A/B는 나머지 공간 사용

---

## 3. Flash API 사용법

### 3.1 Flash Type Enum

```c
// IfxFlash_cfg_TC37x.h
typedef enum
{
    IfxFlash_FlashType_D0 = 0,   // Data Flash Bank 0
    IfxFlash_FlashType_D1 = 1,   // Data Flash Bank 1
    IfxFlash_FlashType_P0 = 2,   // Program Flash Bank 0
    IfxFlash_FlashType_P1 = 3,   // Program Flash Bank 1
    IfxFlash_FlashType_Fa = 16   // Program Flash Array
} IfxFlash_FlashType;
```

### 3.2 핵심 Flash API

#### 3.2.1 Erase Operations

```c
// 단일 Logical Sector 삭제
void IfxFlash_eraseSector(uint32 sectorAddr);

// 여러 연속된 Logical Sector 삭제
void IfxFlash_eraseMultipleSectors(uint32 sectorAddr, uint32 numSector);

// 예제:
// Bank 0의 첫 번째 Logical Sector (0xA0000000 ~ 0xA0003FFF) 삭제
IfxFlash_eraseSector(0xA0000000);

// Bank 1의 첫 3개 Logical Sector 삭제
IfxFlash_eraseMultipleSectors(0xA0300000, 3);
```

#### 3.2.2 Page Mode 진입

```c
// Page Mode 진입 (Write 작업 전 필수)
uint8 IfxFlash_enterPageMode(uint32 pageAddr);

// 반환값:
//   0 = Success
//   1 = Invalid flash address

// 예제:
uint32 pageAddr = 0xA0000000;
if (IfxFlash_enterPageMode(pageAddr) == 0) {
    // Page mode 진입 성공
}
```

**Page Mode란?**
- Flash에 데이터를 쓰기 전에 반드시 진입해야 하는 모드
- PFLASH: 페이지당 32 Bytes
- DFLASH: 페이지당 8 Bytes

#### 3.2.3 Data Load

```c
// 64비트(8 Bytes) 데이터 로드 - 단일 64비트 접근
void IfxFlash_loadPage(uint32 pageAddr, uint32 wordL, uint32 wordU);

// 64비트(8 Bytes) 데이터 로드 - 두 번의 32비트 접근
void IfxFlash_loadPage2X32(uint32 pageAddr, uint32 wordL, uint32 wordU);

// 예제:
// PFLASH Page (32 Bytes) 채우기
uint32 pageAddr = 0xA0000000;
for (int offset = 0; offset < 32; offset += 8) {
    IfxFlash_loadPage2X32(pageAddr + offset, 0x12345678, 0xABCDEF00);
}
```

**wordL vs wordU:**
- `wordL`: Lower Address Word (하위 4 Bytes)
- `wordU`: Upper Address Word (상위 4 Bytes)
- 메모리 레이아웃: `[wordL (32bit)][wordU (32bit)]` = 64bit

#### 3.2.4 Write Operations

```c
// 페이지 쓰기
void IfxFlash_writePage(uint32 pageAddr);

// 페이지 쓰기 + 검증
void IfxFlash_writePageOnce(uint32 pageAddr);

// Burst 쓰기 (더 빠름, 연속된 여러 페이지)
void IfxFlash_writeBurst(uint32 pageAddr);

// 예제:
IfxFlash_writePage(0xA0000000);
```

#### 3.2.5 Wait for Completion

```c
// Flash 작업이 완료될 때까지 대기
uint8 IfxFlash_waitUnbusy(uint32 flash, IfxFlash_FlashType flashType);

// flash: 0 (모듈 번호, TC375는 1개만 있음)
// flashType: P0, P1, D0, D1

// 예제:
// PFLASH Bank 0 작업 완료 대기
IfxFlash_waitUnbusy(0, IfxFlash_FlashType_P0);

// DFLASH Bank 0 작업 완료 대기
IfxFlash_waitUnbusy(0, IfxFlash_FlashType_D0);
```

#### 3.2.6 Status & Error Handling

```c
// 상태 플래그 클리어
void IfxFlash_clearStatus(uint32 flash);

// Read 모드로 리셋
void IfxFlash_resetToRead(uint32 flash);

// 모든 Flash Bank가 Busy가 아닐 때까지 대기
boolean IfxFlash_waitUnbusyAll(void);
```

### 3.3 완전한 Flash Programming 시퀀스

```c
/*
 * Flash Programming의 기본 시퀀스
 * 
 * 1. Erase Logical Sector
 * 2. Wait until Flash is ready
 * 3. Enter Page Mode
 * 4. Wait until Flash is ready
 * 5. Load data into page buffer
 * 6. Write Page
 * 7. Wait until Flash is ready
 */

// 1. Erase
uint16 password = IfxScuWdt_getSafetyWatchdogPassword();
IfxScuWdt_clearSafetyEndinit(password);
IfxFlash_eraseMultipleSectors(0xA0000000, 1);  // 첫 Logical Sector 삭제
IfxScuWdt_setSafetyEndinit(password);

// 2. Wait
IfxFlash_waitUnbusy(0, IfxFlash_FlashType_P0);

// 3. Enter Page Mode
IfxFlash_enterPageMode(0xA0000000);

// 4. Wait
IfxFlash_waitUnbusy(0, IfxFlash_FlashType_P0);

// 5. Load Data (PFLASH Page = 32 Bytes = 4 × 64-bit)
for (int offset = 0; offset < 32; offset += 8) {
    IfxFlash_loadPage2X32(0xA0000000 + offset, 0xDEADBEEF, 0xCAFEBABE);
}

// 6. Write
IfxScuWdt_clearSafetyEndinit(password);
IfxFlash_writePage(0xA0000000);
IfxScuWdt_setSafetyEndinit(password);

// 7. Wait
IfxFlash_waitUnbusy(0, IfxFlash_FlashType_P0);
```

---

## 4. PSPR (Program Scratch-Pad RAM) 사용

### 4.1 PSPR이 필요한 이유

**⚠️ 중요한 제약사항:**
> **같은 PFLASH Bank에서 실행 중인 코드는 그 Bank를 프로그래밍할 수 없습니다!**

- 예: Bank 0 (0x80000000~)에서 실행 중인 코드로 Bank 0을 Write/Erase 불가
- 이유: Flash Controller가 Read와 Write를 동시에 수행할 수 없음
- 해결: Flash 함수를 **PSPR (RAM)**로 복사한 후 실행

### 4.2 PSPR 메모리 레이아웃

```c
// CPU0 PSPR
#define PSPR0_BASE          0x70100000
#define PSPR0_SIZE          64KB (0x10000)

// 함수 복사를 위한 주소 할당 예제
#define ERASE_FUNC_ADDR     0x70100000    // 110 bytes
#define WAIT_FUNC_ADDR      0x7010006E    // 110 bytes (0x70100000 + 110)
#define ENTER_PAGE_ADDR     0x701000DC    // 110 bytes
#define LOAD_PAGE_ADDR      0x7010014A    // 110 bytes
#define WRITE_PAGE_ADDR     0x701001B8    // 110 bytes
```

### 4.3 함수 복사 방법

#### 4.3.1 memcpy를 이용한 복사

```c
#include <string.h>

// 함수 포인터 타입 정의
typedef void (*EraseSectorFunc)(uint32 sectorAddr, uint32 numSector);
typedef uint8 (*WaitUnbusyFunc)(uint32 flash, IfxFlash_FlashType flashType);
typedef uint8 (*EnterPageModeFunc)(uint32 pageAddr);
typedef void (*LoadPage2X32Func)(uint32 pageAddr, uint32 wordL, uint32 wordU);
typedef void (*WritePageFunc)(uint32 pageAddr);

// 함수 포인터 변수
EraseSectorFunc     eraseSectorFromPSPR;
WaitUnbusyFunc      waitUnbusyFromPSPR;
EnterPageModeFunc   enterPageModeFromPSPR;
LoadPage2X32Func    loadPage2X32FromPSPR;
WritePageFunc       writePageFromPSPR;

// 함수 복사
void copyFlashFunctionsToPSPR(void)
{
    // IfxFlash_eraseMultipleSectors 복사
    memcpy((void *)ERASE_FUNC_ADDR, 
           (const void *)IfxFlash_eraseMultipleSectors, 
           110);  // 110 bytes는 예측 크기
    eraseSectorFromPSPR = (EraseSectorFunc)ERASE_FUNC_ADDR;
    
    // IfxFlash_waitUnbusy 복사
    memcpy((void *)WAIT_FUNC_ADDR, 
           (const void *)IfxFlash_waitUnbusy, 
           110);
    waitUnbusyFromPSPR = (WaitUnbusyFunc)WAIT_FUNC_ADDR;
    
    // IfxFlash_enterPageMode 복사
    memcpy((void *)ENTER_PAGE_ADDR, 
           (const void *)IfxFlash_enterPageMode, 
           110);
    enterPageModeFromPSPR = (EnterPageModeFunc)ENTER_PAGE_ADDR;
    
    // IfxFlash_loadPage2X32 복사
    memcpy((void *)LOAD_PAGE_ADDR, 
           (const void *)IfxFlash_loadPage2X32, 
           110);
    loadPage2X32FromPSPR = (LoadPage2X32Func)LOAD_PAGE_ADDR;
    
    // IfxFlash_writePage 복사
    memcpy((void *)WRITE_PAGE_ADDR, 
           (const void *)IfxFlash_writePage, 
           110);
    writePageFromPSPR = (WritePageFunc)WRITE_PAGE_ADDR;
}
```

#### 4.3.2 함수 크기 추정

```c
// Flash_Programming.c에서 사용된 크기 (예측치)
#define ERASESECTOR_LEN      110
#define WAITUNBUSY_LEN       110
#define ENTERPAGEMODE_LEN    110
#define LOADPAGE2X32_LEN     110
#define WRITEPAGE_LEN        110

// 더 큰 Wrapper 함수
#define ERASEPFLASH_LEN      0x186  // 390 bytes
#define WRITEPFLASH_LEN      0x228  // 552 bytes
```

**주의사항:**
- 함수 크기는 컴파일러와 최적화 레벨에 따라 변함
- 충분한 여유 공간 확보 권장 (실제 크기 × 1.5~2배)
- 함수가 다른 함수를 호출하는 경우, 그 함수도 PSPR로 복사해야 함

#### 4.3.3 PSPR에서 실행 가능한 함수

**PSPR에서 호출 가능:**
- PSPR로 복사한 다른 함수
- Inline 함수 (e.g., `IfxScuWdt_clearSafetyEndinitInline`)
- CPU Intrinsic 함수 (e.g., `__dsync()`)

**PSPR에서 호출 불가:**
- PFLASH에 있는 일반 함수
- 이유: CTYP (Context Type) Trap 발생

### 4.4 PSPR 사용 예제 (완전한 함수)

```c
// PSPR로 복사될 Erase 함수 (PFLASH에서 정의)
void erasePFLASH(uint32 sectorAddr)
{
    // Inline 함수는 사용 가능
    uint16 password = IfxScuWdt_getSafetyWatchdogPasswordInline();
    
    // EndInit Protection 해제
    IfxScuWdt_clearSafetyEndinitInline(password);
    
    // PSPR로 복사한 함수 호출
    eraseSectorFromPSPR(sectorAddr, 1);
    
    // EndInit Protection 활성화
    IfxScuWdt_setSafetyEndinitInline(password);
    
    // PSPR로 복사한 Wait 함수 호출
    waitUnbusyFromPSPR(0, IfxFlash_FlashType_P0);
}

// PSPR로 복사될 Write 함수 (PFLASH에서 정의)
void writePFLASH(uint32 startAddr, uint8 *data, uint32 length)
{
    uint16 password = IfxScuWdt_getSafetyWatchdogPasswordInline();
    uint32 numPages = (length + 31) / 32;  // PFLASH Page = 32 Bytes
    
    for (uint32 page = 0; page < numPages; page++)
    {
        uint32 pageAddr = startAddr + (page * 32);
        
        // Enter Page Mode
        enterPageModeFromPSPR(pageAddr);
        waitUnbusyFromPSPR(0, IfxFlash_FlashType_P0);
        
        // Load Data (32 Bytes = 4 × 8 Bytes)
        for (uint32 offset = 0; offset < 32; offset += 8)
        {
            uint32 idx = page * 32 + offset;
            uint32 wordL = *(uint32 *)(&data[idx]);
            uint32 wordU = *(uint32 *)(&data[idx + 4]);
            loadPage2X32FromPSPR(pageAddr + offset, wordL, wordU);
        }
        
        // Write Page
        IfxScuWdt_clearSafetyEndinitInline(password);
        writePageFromPSPR(pageAddr);
        IfxScuWdt_setSafetyEndinitInline(password);
        
        // Wait
        waitUnbusyFromPSPR(0, IfxFlash_FlashType_P0);
    }
}

// 메인 함수에서 사용
void updateBankB(uint8 *firmwareData, uint32 firmwareSize)
{
    // 1. 인터럽트 비활성화
    boolean interruptState = IfxCpu_disableInterrupts();
    
    // 2. Flash 함수들을 PSPR로 복사
    copyFlashFunctionsToPSPR();
    
    // 3. 추가로 Wrapper 함수들도 PSPR로 복사
    memcpy((void *)ERASEPFLASH_ADDR, (const void *)erasePFLASH, 0x186);
    memcpy((void *)WRITEPFLASH_ADDR, (const void *)writePFLASH, 0x228);
    
    void (*eraseFromPSPR)(uint32) = (void (*)(uint32))ERASEPFLASH_ADDR;
    void (*writeFromPSPR)(uint32, uint8*, uint32) = 
        (void (*)(uint32, uint8*, uint32))WRITEPFLASH_ADDR;
    
    // 4. Bank B Erase
    eraseFromPSPR(APP_BANK_B_START);
    
    // 5. Bank B Write
    writeFromPSPR(APP_BANK_B_START, firmwareData, firmwareSize);
    
    // 6. 인터럽트 복원
    IfxCpu_restoreInterrupts(interruptState);
}
```

---

## 5. EndInit Protection

### 5.1 EndInit Protection이란?

- TC375의 **Safety Watchdog**에 의한 보호 메커니즘
- Flash Write/Erase 같은 중요한 작업은 EndInit Protection을 해제해야 수행 가능
- 보안 및 안정성 향상을 위한 기능

### 5.2 EndInit Protection 사용법

```c
#include "IfxScuWdt.h"

// 1. Password 획득
uint16 password = IfxScuWdt_getSafetyWatchdogPassword();

// 2. EndInit Protection 해제
IfxScuWdt_clearSafetyEndinit(password);

// 3. 보호된 작업 수행 (Flash Write/Erase)
IfxFlash_writePage(0xA0000000);

// 4. EndInit Protection 재활성화
IfxScuWdt_setSafetyEndinit(password);
```

### 5.3 Inline 버전 (PSPR에서 사용 가능)

```c
// PSPR에서 실행되는 함수 내부에서는 Inline 버전 사용
uint16 password = IfxScuWdt_getSafetyWatchdogPasswordInline();

IfxScuWdt_clearSafetyEndinitInline(password);
// ... Flash 작업 ...
IfxScuWdt_setSafetyEndinitInline(password);
```

### 5.4 주의사항

- **EndInit 해제 시간을 최소화**할 것
- 해제 후 반드시 재활성화
- 일반적인 패턴:
  ```c
  clear → single_flash_operation → set
  ```
- 여러 작업을 한 번에 하지 말고, 작업마다 clear/set 반복

---

## 6. Linker Script 설정

### 6.1 TASKING Linker Script (Lcf_Tasking_Tricore_Tc.lsl)

#### 6.1.1 메모리 정의

```c
// Memory Map 정의
memory pfls0    // Program Flash Bank 0
{
    mau = 8;
    size = 3M;
    type = rom;
    map     cached (dest=bus:sri, dest_offset=0x80000000, size=3M);
    map not_cached (dest=bus:sri, dest_offset=0xa0000000, reserved, size=3M);
}

memory pfls1    // Program Flash Bank 1
{
    mau = 8;
    size = 3M;
    type = rom;
    map     cached (dest=bus:sri, dest_offset=0x80300000, size=3M);
    map not_cached (dest=bus:sri, dest_offset=0xa0300000, reserved, size=3M);
}

memory psram0   // CPU0 Program Scratch-Pad RAM
{
    mau = 8;
    size = 64k;
    type = ram;
    map (dest=bus:tc0:fpi_bus, dest_offset=0xc0000000, size=64k, priority=8);
    map (dest=bus:sri, dest_offset=0x70100000, size=64k);
}

memory dfls0    // Data Flash 0
{
    mau = 8;
    size = 256K;
    type = reserved nvram;
    map (dest=bus:sri, dest_offset=0xaf000000, size=256K);
}

memory cpu0_dlmu    // CPU0 Local Memory (RAM)
{
    mau = 8;
    size = 64k;
    type = ram;
    map     cached (dest=bus:sri, dest_offset=0x90000000, size=64k);
    map not_cached (dest=bus:sri, dest_offset=0xb0000000, reserved, size=64k);
}
```

#### 6.1.2 듀얼 뱅크를 위한 Linker Script 수정

```c
// 듀얼 뱅크 OTA를 위한 Memory Layout

// 1. SSW (Startup Software) - 고정
#define LCF_SSW_START           0x80000000
#define LCF_SSW_SIZE            0x00004000      // 16KB

// 2. Bootloader Bank A
#define LCF_BOOTLOADER_A_START  0x80004000
#define LCF_BOOTLOADER_A_SIZE   0x00020000      // 128KB

// 3. Application Bank A (현재 실행 Bank)
#define LCF_APP_A_START         0x80024000
#define LCF_APP_A_SIZE          0x002DC000      // ~2.85MB

// 4. Bootloader Bank B
#define LCF_BOOTLOADER_B_START  0x80300000
#define LCF_BOOTLOADER_B_SIZE   0x00020000      // 128KB

// 5. Application Bank B (OTA 업데이트 Bank)
#define LCF_APP_B_START         0x80320000
#define LCF_APP_B_SIZE          0x002E0000      // ~2.87MB

// 6. Interrupt Vector 위치
#define LCF_INTVEC0_START       (LCF_APP_A_START + LCF_APP_A_SIZE - 0x2000)
#define LCF_TRAPVEC0_START      (LCF_APP_A_START + 0x100)

// 7. RAM 레이아웃
#define LCF_DSPR0_START         0x70000000
#define LCF_DSPR0_SIZE          240k

#define LCF_CSA0_SIZE           64k             // Context Save Area (증가)
#define LCF_USTACK0_SIZE        16k             // User Stack (증가)
#define LCF_ISTACK0_SIZE        8k              // Interrupt Stack (증가)
#define LCF_HEAP_SIZE           64k             // Heap (lwIP용, 증가)
```

### 6.2 GCC Linker Script (Lcf_Gnuc_Tricore_Tc.lsl)

#### 6.2.1 메모리 정의

```c
MEMORY
{
    psram0 (w!xp):      org = 0x70100000, len = 64K
    dsram0 (w!xp):      org = 0x70000000, len = 240K
    
    pfls0 (rx!p):       org = 0x80000000, len = 3M
    pfls0_nc (rx!p):    org = 0xa0000000, len = 3M
    
    pfls1 (rx!p):       org = 0x80300000, len = 3M
    pfls1_nc (rx!p):    org = 0xa0300000, len = 3M
    
    dfls0 (rx!p):       org = 0xaf000000, len = 256K
    ucb (rx!p):         org = 0xaf400000, len = 24K
    
    cpu0_dlmu (w!xp):   org = 0x90000000, len = 64K
    cpu0_dlmu_nc (w!xp): org = 0xb0000000, len = 64K
}
```

#### 6.2.2 듀얼 뱅크 Section 정의

```c
// Bank A (Active Application)
LCF_APP_A_START = 0x80024000;
LCF_APP_A_SIZE = 0x002DC000;

// Bank B (OTA Update Target)
LCF_APP_B_START = 0x80320000;
LCF_APP_B_SIZE = 0x002E0000;

// Section 배치
SECTIONS
{
    /* Application Code - Bank A */
    .text LCF_APP_A_START :
    {
        *(.text.core0_main)
        *(.text*)
        *(.rodata*)
    } > pfls0
    
    /* Data in RAM */
    .data :
    {
        *(.data*)
    } > dsram0
    
    .bss :
    {
        *(.bss*)
        *(COMMON)
    } > dsram0
    
    /* PSPR for Flash Functions */
    .pspr_text 0x70100000 :
    {
        *(.pspr_text)
    } > psram0
}
```

---

## 7. 실전 코드 예제

### 7.1 PFLASH Bank 1 Erase & Write (완전한 예제)

```c
/******************************************************************************
 * File: flash_update.c
 * Description: Bank 1 (PF1) Update for OTA
 ******************************************************************************/

#include <string.h>
#include "Ifx_Types.h"
#include "IfxFlash.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"

/* ========================= Configuration ========================= */

#define PFLASH_PAGE_LENGTH      32              // PFLASH Page = 32 Bytes
#define PFLASH_SECTOR_SIZE      (16 * 1024)     // Logical Sector = 16KB
#define FLASH_MODULE            0               // Flash Module ID

// Bank B (PF1) Memory Map
#define BANK_B_START            0xA0320000      // Cached 주소
#define BANK_B_SIZE             0x002E0000      // ~2.87MB
#define BANK_B_SECTOR_COUNT     (BANK_B_SIZE / PFLASH_SECTOR_SIZE)  // ~186 sectors

// PSPR Layout
#define PSPR_BASE               0x70100000
#define ERASE_FUNC_SIZE         150
#define WAIT_FUNC_SIZE          150
#define ENTER_PAGE_SIZE         150
#define LOAD_PAGE_SIZE          150
#define WRITE_PAGE_SIZE         150

#define ERASE_FUNC_ADDR         (PSPR_BASE)
#define WAIT_FUNC_ADDR          (ERASE_FUNC_ADDR + ERASE_FUNC_SIZE)
#define ENTER_PAGE_ADDR         (WAIT_FUNC_ADDR + WAIT_FUNC_SIZE)
#define LOAD_PAGE_ADDR          (ENTER_PAGE_ADDR + ENTER_PAGE_SIZE)
#define WRITE_PAGE_ADDR         (LOAD_PAGE_ADDR + LOAD_PAGE_SIZE)

/* ========================= Type Definitions ========================= */

typedef void  (*EraseSectorsFunc)(uint32, uint32);
typedef uint8 (*WaitUnbusyFunc)(uint32, IfxFlash_FlashType);
typedef uint8 (*EnterPageModeFunc)(uint32);
typedef void  (*LoadPage2X32Func)(uint32, uint32, uint32);
typedef void  (*WritePageFunc)(uint32);

typedef struct {
    EraseSectorsFunc  eraseSectors;
    WaitUnbusyFunc    waitUnbusy;
    EnterPageModeFunc enterPageMode;
    LoadPage2X32Func  loadPage2X32;
    WritePageFunc     writePage;
} FlashFunctions;

/* ========================= Global Variables ========================= */

static FlashFunctions g_flashFuncs;

/* ========================= Function Prototypes ========================= */

static void copyFlashFunctionsToPSPR(void);
static boolean eraseBank(uint32 bankStart, uint32 numSectors);
static boolean writeBank(uint32 bankStart, const uint8 *data, uint32 length);
static boolean verifyBank(uint32 bankStart, const uint8 *data, uint32 length);

/* ========================= Public Functions ========================= */

/**
 * @brief Bank B (PF1)를 새 펌웨어로 업데이트
 * @param firmwareData 펌웨어 데이터 버퍼
 * @param firmwareSize 펌웨어 크기 (Bytes)
 * @return TRUE on success, FALSE on failure
 */
boolean updateBankB(const uint8 *firmwareData, uint32 firmwareSize)
{
    boolean success = FALSE;
    
    // Validation
    if (firmwareData == NULL || firmwareSize == 0 || firmwareSize > BANK_B_SIZE)
    {
        return FALSE;
    }
    
    // 1. 인터럽트 비활성화
    boolean interruptState = IfxCpu_disableInterrupts();
    
    // 2. Flash 함수들을 PSPR로 복사
    copyFlashFunctionsToPSPR();
    
    // 3. Bank B Erase
    uint32 numSectors = (firmwareSize + PFLASH_SECTOR_SIZE - 1) / PFLASH_SECTOR_SIZE;
    if (!eraseBank(BANK_B_START, numSectors))
    {
        goto cleanup;
    }
    
    // 4. Bank B Write
    if (!writeBank(BANK_B_START, firmwareData, firmwareSize))
    {
        goto cleanup;
    }
    
    // 5. Verify
    if (!verifyBank(BANK_B_START, firmwareData, firmwareSize))
    {
        goto cleanup;
    }
    
    success = TRUE;

cleanup:
    // 6. 인터럽트 복원
    IfxCpu_restoreInterrupts(interruptState);
    
    return success;
}

/* ========================= Private Functions ========================= */

/**
 * @brief Flash API 함수들을 PSPR로 복사
 */
static void copyFlashFunctionsToPSPR(void)
{
    // iLLD API 함수들을 PSPR로 복사
    memcpy((void *)ERASE_FUNC_ADDR, 
           (const void *)IfxFlash_eraseMultipleSectors, 
           ERASE_FUNC_SIZE);
    g_flashFuncs.eraseSectors = (EraseSectorsFunc)ERASE_FUNC_ADDR;
    
    memcpy((void *)WAIT_FUNC_ADDR, 
           (const void *)IfxFlash_waitUnbusy, 
           WAIT_FUNC_SIZE);
    g_flashFuncs.waitUnbusy = (WaitUnbusyFunc)WAIT_FUNC_ADDR;
    
    memcpy((void *)ENTER_PAGE_ADDR, 
           (const void *)IfxFlash_enterPageMode, 
           ENTER_PAGE_SIZE);
    g_flashFuncs.enterPageMode = (EnterPageModeFunc)ENTER_PAGE_ADDR;
    
    memcpy((void *)LOAD_PAGE_ADDR, 
           (const void *)IfxFlash_loadPage2X32, 
           LOAD_PAGE_SIZE);
    g_flashFuncs.loadPage2X32 = (LoadPage2X32Func)LOAD_PAGE_ADDR;
    
    memcpy((void *)WRITE_PAGE_ADDR, 
           (const void *)IfxFlash_writePage, 
           WRITE_PAGE_SIZE);
    g_flashFuncs.writePage = (WritePageFunc)WRITE_PAGE_ADDR;
}

/**
 * @brief Bank Erase
 */
static boolean eraseBank(uint32 bankStart, uint32 numSectors)
{
    uint16 password = IfxScuWdt_getSafetyWatchdogPasswordInline();
    
    // Erase
    IfxScuWdt_clearSafetyEndinitInline(password);
    g_flashFuncs.eraseSectors(bankStart, numSectors);
    IfxScuWdt_setSafetyEndinitInline(password);
    
    // Wait
    uint8 result = g_flashFuncs.waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_P1);
    
    return (result == 0);
}

/**
 * @brief Bank Write
 */
static boolean writeBank(uint32 bankStart, const uint8 *data, uint32 length)
{
    uint16 password = IfxScuWdt_getSafetyWatchdogPasswordInline();
    uint32 numPages = (length + PFLASH_PAGE_LENGTH - 1) / PFLASH_PAGE_LENGTH;
    
    for (uint32 page = 0; page < numPages; page++)
    {
        uint32 pageAddr = bankStart + (page * PFLASH_PAGE_LENGTH);
        
        // Enter Page Mode
        if (g_flashFuncs.enterPageMode(pageAddr) != 0)
        {
            return FALSE;
        }
        g_flashFuncs.waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_P1);
        
        // Load Page Data (32 Bytes = 4 × 8 Bytes)
        for (uint32 offset = 0; offset < PFLASH_PAGE_LENGTH; offset += 8)
        {
            uint32 dataIdx = (page * PFLASH_PAGE_LENGTH) + offset;
            uint32 wordL = 0xFFFFFFFF;  // Default erased value
            uint32 wordU = 0xFFFFFFFF;
            
            if (dataIdx < length)
            {
                // wordL = data[dataIdx] ~ data[dataIdx+3]
                wordL = *(uint32 *)(&data[dataIdx]);
            }
            if (dataIdx + 4 < length)
            {
                // wordU = data[dataIdx+4] ~ data[dataIdx+7]
                wordU = *(uint32 *)(&data[dataIdx + 4]);
            }
            
            g_flashFuncs.loadPage2X32(pageAddr + offset, wordL, wordU);
        }
        
        // Write Page
        IfxScuWdt_clearSafetyEndinitInline(password);
        g_flashFuncs.writePage(pageAddr);
        IfxScuWdt_setSafetyEndinitInline(password);
        
        // Wait
        g_flashFuncs.waitUnbusy(FLASH_MODULE, IfxFlash_FlashType_P1);
    }
    
    return TRUE;
}

/**
 * @brief Bank Verify
 */
static boolean verifyBank(uint32 bankStart, const uint8 *data, uint32 length)
{
    volatile uint8 *flashData = (volatile uint8 *)bankStart;
    
    for (uint32 i = 0; i < length; i++)
    {
        if (flashData[i] != data[i])
        {
            return FALSE;  // Verification failed
        }
    }
    
    return TRUE;
}
```

### 7.2 DFLASH Write (설정 데이터 저장)

```c
/******************************************************************************
 * File: config_storage.c
 * Description: Configuration data storage in DFLASH
 ******************************************************************************/

#include "Ifx_Types.h"
#include "IfxFlash.h"
#include "IfxScuWdt.h"

/* ========================= Configuration ========================= */

#define DFLASH_PAGE_LENGTH      8               // DFLASH Page = 8 Bytes
#define DFLASH_SECTOR_SIZE      (4 * 1024)      // DFLASH Sector = 4KB

// Configuration Storage in DFLASH
#define CONFIG_SECTOR_ADDR      0xAF000000      // First DFLASH Sector
#define CONFIG_MAX_SIZE         DFLASH_SECTOR_SIZE

/* ========================= Public Functions ========================= */

/**
 * @brief 설정 데이터를 DFLASH에 저장
 * @param configData 설정 데이터 버퍼
 * @param configSize 데이터 크기 (Bytes)
 * @return TRUE on success, FALSE on failure
 */
boolean saveConfigToDFLASH(const uint8 *configData, uint32 configSize)
{
    if (configData == NULL || configSize == 0 || configSize > CONFIG_MAX_SIZE)
    {
        return FALSE;
    }
    
    uint16 password = IfxScuWdt_getSafetyWatchdogPassword();
    uint32 numPages = (configSize + DFLASH_PAGE_LENGTH - 1) / DFLASH_PAGE_LENGTH;
    
    // 1. Erase Sector
    IfxScuWdt_clearSafetyEndinit(password);
    IfxFlash_eraseMultipleSectors(CONFIG_SECTOR_ADDR, 1);
    IfxScuWdt_setSafetyEndinit(password);
    IfxFlash_waitUnbusy(0, IfxFlash_FlashType_D0);
    
    // 2. Write Pages
    for (uint32 page = 0; page < numPages; page++)
    {
        uint32 pageAddr = CONFIG_SECTOR_ADDR + (page * DFLASH_PAGE_LENGTH);
        
        // Enter Page Mode
        IfxFlash_enterPageMode(pageAddr);
        IfxFlash_waitUnbusy(0, IfxFlash_FlashType_D0);
        
        // Load Data (8 Bytes = 1 × 8 Bytes)
        uint32 dataIdx = page * DFLASH_PAGE_LENGTH;
        uint32 wordL = 0xFFFFFFFF;
        uint32 wordU = 0xFFFFFFFF;
        
        if (dataIdx < configSize)
        {
            wordL = *(uint32 *)(&configData[dataIdx]);
        }
        if (dataIdx + 4 < configSize)
        {
            wordU = *(uint32 *)(&configData[dataIdx + 4]);
        }
        
        IfxFlash_loadPage2X32(pageAddr, wordL, wordU);
        
        // Write Page
        IfxScuWdt_clearSafetyEndinit(password);
        IfxFlash_writePage(pageAddr);
        IfxScuWdt_setSafetyEndinit(password);
        IfxFlash_waitUnbusy(0, IfxFlash_FlashType_D0);
    }
    
    // 3. Verify
    volatile uint8 *flashData = (volatile uint8 *)CONFIG_SECTOR_ADDR;
    for (uint32 i = 0; i < configSize; i++)
    {
        if (flashData[i] != configData[i])
        {
            return FALSE;
        }
    }
    
    return TRUE;
}

/**
 * @brief DFLASH에서 설정 데이터 읽기
 */
boolean loadConfigFromDFLASH(uint8 *configData, uint32 configSize)
{
    if (configData == NULL || configSize == 0 || configSize > CONFIG_MAX_SIZE)
    {
        return FALSE;
    }
    
    volatile uint8 *flashData = (volatile uint8 *)CONFIG_SECTOR_ADDR;
    
    for (uint32 i = 0; i < configSize; i++)
    {
        configData[i] = flashData[i];
    }
    
    return TRUE;
}
```

---

## 8. 듀얼 뱅크 OTA 구현 가이드

### 8.1 듀얼 뱅크 Boot 시퀀스

```
Power-On Reset
    |
    v
SSW (0x80000000)          <--- BMI (Boot Mode Index)로 부팅 위치 결정
    |
    v
Bootloader A or B?        <--- UCB 또는 DFLASH의 Boot Flag 확인
    |
    +--- Bank A Active ----> Bootloader A (0x80004000)
    |                           |
    |                           v
    |                        Application A (0x80024000) 실행
    |
    +--- Bank B Active ----> Bootloader B (0x80300000)
                                |
                                v
                             Application B (0x80320000) 실행
```

### 8.2 OTA Update Flow

```
Step 1: VMG로부터 새 펌웨어 다운로드
    ├─> zone.bin 수신
    ├─> CRC/Checksum 검증
    └─> RAM 버퍼에 저장

Step 2: 현재 실행 중인 Bank 확인
    ├─> Bank A 실행 중 → Bank B로 업데이트
    └─> Bank B 실행 중 → Bank A로 업데이트

Step 3: 비활성 Bank Erase & Write
    ├─> PSPR로 Flash 함수 복사
    ├─> 비활성 Bank Erase
    ├─> 새 펌웨어 Write
    └─> Verification

Step 4: Boot Flag 변경
    ├─> DFLASH에 새 Boot Bank 정보 기록
    └─> CRC 저장

Step 5: System Reset
    ├─> 소프트웨어 리셋 수행
    └─> 새 Bank에서 부팅

Step 6: Rollback 메커니즘
    ├─> 새 펌웨어 부팅 후 Self-Test
    ├─> 실패 시 Boot Flag를 이전 Bank로 복원
    └─> 다시 리셋
```

### 8.3 Boot Flag 구조체 (DFLASH에 저장)

```c
#define BOOT_FLAG_MAGIC         0xA5C3E7B1      // Magic Number

typedef enum {
    BOOT_BANK_A = 0,
    BOOT_BANK_B = 1
} BootBank_t;

typedef struct {
    uint32      magic;              // BOOT_FLAG_MAGIC
    BootBank_t  activeBank;         // 현재 활성 Bank
    uint32      bankAVersion;       // Bank A 펌웨어 버전
    uint32      bankBVersion;       // Bank B 펌웨어 버전
    uint32      bankABootCount;     // Bank A 부팅 시도 횟수
    uint32      bankBBootCount;     // Bank B 부팅 시도 횟수
    uint32      maxBootRetries;     // 최대 재시도 횟수 (예: 3)
    uint32      crc32;              // 구조체 CRC32
} BootConfig_t;

// DFLASH 주소
#define BOOT_CONFIG_ADDR        0xAF001000      // DFLASH Sector 1
```

### 8.4 Boot Flag 관리 함수

```c
/**
 * @brief Boot Config 읽기
 */
boolean readBootConfig(BootConfig_t *config)
{
    volatile BootConfig_t *flashConfig = (volatile BootConfig_t *)BOOT_CONFIG_ADDR;
    
    // Read from DFLASH
    memcpy(config, (const void *)flashConfig, sizeof(BootConfig_t));
    
    // Validate Magic & CRC
    if (config->magic != BOOT_FLAG_MAGIC)
    {
        return FALSE;
    }
    
    uint32 calculatedCRC = calculateCRC32((uint8 *)config, 
                                           sizeof(BootConfig_t) - 4);
    if (calculatedCRC != config->crc32)
    {
        return FALSE;
    }
    
    return TRUE;
}

/**
 * @brief Boot Config 쓰기
 */
boolean writeBootConfig(const BootConfig_t *config)
{
    BootConfig_t tempConfig;
    memcpy(&tempConfig, config, sizeof(BootConfig_t));
    
    // Calculate CRC
    tempConfig.crc32 = calculateCRC32((uint8 *)&tempConfig, 
                                       sizeof(BootConfig_t) - 4);
    
    // Write to DFLASH
    return saveConfigToDFLASH((const uint8 *)&tempConfig, sizeof(BootConfig_t));
}

/**
 * @brief 다음 부팅 시 Bank 전환
 */
boolean switchToBank(BootBank_t targetBank)
{
    BootConfig_t config;
    
    if (!readBootConfig(&config))
    {
        // 초기화
        config.magic = BOOT_FLAG_MAGIC;
        config.activeBank = BOOT_BANK_A;
        config.bankAVersion = 0;
        config.bankBVersion = 0;
        config.bankABootCount = 0;
        config.bankBBootCount = 0;
        config.maxBootRetries = 3;
    }
    
    // Change active bank
    config.activeBank = targetBank;
    
    // Reset boot count for target bank
    if (targetBank == BOOT_BANK_A)
    {
        config.bankABootCount = 0;
    }
    else
    {
        config.bankBBootCount = 0;
    }
    
    return writeBootConfig(&config);
}
```

### 8.5 Bootloader 로직

```c
/**
 * @brief Bootloader 메인 로직 (SSW 이후 실행)
 */
void bootloader_main(void)
{
    BootConfig_t config;
    boolean configValid = readBootConfig(&config);
    
    if (!configValid)
    {
        // Default: Boot from Bank A
        config.activeBank = BOOT_BANK_A;
        config.bankABootCount = 0;
        config.bankBBootCount = 0;
        config.maxBootRetries = 3;
        writeBootConfig(&config);
    }
    
    // 현재 Bank의 Boot Count 증가
    if (config.activeBank == BOOT_BANK_A)
    {
        config.bankABootCount++;
    }
    else
    {
        config.bankBBootCount++;
    }
    
    // Boot Count가 Max를 초과하면 다른 Bank로 Rollback
    uint32 currentBootCount = (config.activeBank == BOOT_BANK_A) ? 
                               config.bankABootCount : config.bankBBootCount;
    
    if (currentBootCount > config.maxBootRetries)
    {
        // Rollback to other bank
        config.activeBank = (config.activeBank == BOOT_BANK_A) ? 
                             BOOT_BANK_B : BOOT_BANK_A;
        
        if (config.activeBank == BOOT_BANK_A)
        {
            config.bankABootCount = 0;
        }
        else
        {
            config.bankBBootCount = 0;
        }
        
        writeBootConfig(&config);
    }
    else
    {
        // Update boot count
        writeBootConfig(&config);
    }
    
    // Jump to Application
    uint32 appStartAddr;
    if (config.activeBank == BOOT_BANK_A)
    {
        appStartAddr = APP_BANK_A_START;
    }
    else
    {
        appStartAddr = APP_BANK_B_START;
    }
    
    jumpToApplication(appStartAddr);
}

/**
 * @brief Application으로 점프
 */
void jumpToApplication(uint32 appStartAddr)
{
    // Reset Vector 주소 읽기 (Application의 첫 4 Bytes)
    volatile uint32 *resetVectorAddr = (volatile uint32 *)appStartAddr;
    uint32 resetVector = *resetVectorAddr;
    
    // Application의 Reset Handler 호출
    void (*appResetHandler)(void) = (void (*)(void))resetVector;
    appResetHandler();
}
```

### 8.6 Application에서 Self-Test

```c
/**
 * @brief Application 부팅 후 Self-Test
 */
void applicationSelfTest(void)
{
    boolean testPassed = TRUE;
    
    // Test 1: RAM Test
    testPassed &= ramTest();
    
    // Test 2: Communication Test
    testPassed &= comTest();
    
    // Test 3: Timer Test
    testPassed &= timerTest();
    
    if (testPassed)
    {
        // Self-Test 성공 -> Boot Count를 0으로 리셋
        BootConfig_t config;
        if (readBootConfig(&config))
        {
            if (config.activeBank == BOOT_BANK_A)
            {
                config.bankABootCount = 0;
            }
            else
            {
                config.bankBBootCount = 0;
            }
            writeBootConfig(&config);
        }
    }
    else
    {
        // Self-Test 실패 -> 시스템 리셋 (Bootloader가 Rollback 수행)
        systemReset();
    }
}
```

### 8.7 시스템 리셋

```c
#include "IfxScu.h"

/**
 * @brief 소프트웨어 시스템 리셋
 */
void systemReset(void)
{
    // Application Reset 트리거
    IfxScu_performReset(IfxScu_ResetType_application, 0);
    
    // 리셋이 실행되면 이 코드는 도달하지 않음
    while(1);
}
```

---

## 9. 요약 및 체크리스트

### 9.1 Flash Programming 체크리스트

- [ ] **Erase 전 EndInit Protection 해제**
- [ ] **Write 전 EndInit Protection 해제**
- [ ] **Erase 후 `waitUnbusy()` 호출**
- [ ] **Page Mode 진입 후 `waitUnbusy()` 호출**
- [ ] **Write 후 `waitUnbusy()` 호출**
- [ ] **같은 Bank에서 실행하지 않도록 PSPR 사용**
- [ ] **Flash 함수 크기를 충분히 확보 (예측치 × 2배)**
- [ ] **Verification 수행 (Write 후 Read하여 비교)**

### 9.2 듀얼 뱅크 OTA 체크리스트

- [ ] **Linker Script에 Bank A/B 메모리 정의**
- [ ] **Boot Flag를 DFLASH에 저장**
- [ ] **Boot Count 메커니즘 구현 (Rollback용)**
- [ ] **Bootloader에서 Boot Flag 확인**
- [ ] **Application Self-Test 구현**
- [ ] **OTA 업데이트 시 비활성 Bank만 수정**
- [ ] **CRC/Checksum으로 펌웨어 검증**
- [ ] **시스템 리셋 후 새 Bank에서 부팅 확인**

### 9.3 주의사항

1. **PSPR 사용 필수**: 같은 Bank를 프로그래밍할 때는 PSPR에서 실행
2. **EndInit Protection**: Flash 작업 시 반드시 해제 후 재활성화
3. **Wait 함수 호출**: 모든 Flash 작업 후 `waitUnbusy()` 호출
4. **Interrupt 비활성화**: Flash 작업 중에는 인터럽트를 비활성화할 것
5. **Verification**: Write 후 반드시 Verification 수행
6. **Rollback 메커니즘**: OTA 실패 시 이전 Bank로 복구 가능하도록 구현
7. **Boot Count**: 무한 부팅 루프 방지를 위해 Boot Count 제한 설정

---

## 10. 참고 자료

- **Infineon AURIX iLLD Documentation**: `TC37A_iLLD_UM_1_0_1_17_0.chm`
- **Flash_Programming_1_KIT_TC375_LK Example**: Official code example
- **TC37x User Manual**: Flash Memory Chapter
- **AURIX Development Studio**: IDE and Tools

---

**문서 버전**: 1.0  
**작성일**: 2025-11-02  
**대상 프로젝트**: Zonal Gateway Dual-Bank OTA

