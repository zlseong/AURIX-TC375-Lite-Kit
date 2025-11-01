# Zonal Gateway Build Setup Guide

## Overview
This guide helps you configure AURIX Development Studio to build the Zonal Gateway project with FreeRTOS and lwIP.

---

## 📋 **Prerequisites**

- ✅ AURIX Development Studio 1.10.x or later
- ✅ TC375 Lite Kit hardware
- ✅ All dependencies downloaded (lwIP, FreeRTOS) ✓

---

## 🔧 **Step 1: Add Include Paths**

### 1.1 Open Project Properties
```
Right-click project → Properties
→ C/C++ Build → Settings
→ Tool Settings → TASKING C/C++ Compiler → Include Paths
```

### 1.2 Add These Paths

Click **[+]** button and add each path:

```
${workspace_loc:/${ProjName}/lwip/lwip-2.1.3/src/include}
${workspace_loc:/${ProjName}/lwip/lwip-2.1.3/src/include/ipv4}
${workspace_loc:/${ProjName}/lwip/lwip-2.1.3/src/include/netif}
${workspace_loc:/${ProjName}/lwip/port}
${workspace_loc:/${ProjName}/lwip/port/netif}
${workspace_loc:/${ProjName}/FreeRTOS}
${workspace_loc:/${ProjName}/FreeRTOS/FreeRTOS-Kernel/include}
${workspace_loc:/${ProjName}/FreeRTOS/FreeRTOS-Kernel/portable/GCC/TriCore_1782}
```

**Screenshot:** Include paths should look like this:
```
[x] lwip/lwip-2.1.3/src/include
[x] lwip/lwip-2.1.3/src/include/ipv4
[x] lwip/port
[x] FreeRTOS
[x] FreeRTOS/FreeRTOS-Kernel/include
[x] FreeRTOS/FreeRTOS-Kernel/portable/GCC/TriCore_1782
...
```

---

## 📂 **Step 2: Add Source Files to Build**

### 2.1 Option A: Add as Source Folders (Recommended)

```
Right-click project → Properties
→ C/C++ General → Paths and Symbols
→ Source Location tab
→ Add Folder...
```

**Add these folders:**
- `lwip/lwip-2.1.3/src/core`
- `lwip/lwip-2.1.3/src/core/ipv4`
- `lwip/lwip-2.1.3/src/api`
- `lwip/lwip-2.1.3/src/netif`
- `lwip/port`
- `FreeRTOS/FreeRTOS-Kernel` (select specific files below)

### 2.2 Option B: Link Individual Files

Create virtual folders in Eclipse:
```
Right-click project → New → Folder → Advanced → Link to alternate location
```

**lwIP Core Files:**
```
lwip/
└── core/
    ├── init.c
    ├── def.c
    ├── dns.c
    ├── inet_chksum.c
    ├── ip.c
    ├── mem.c
    ├── memp.c
    ├── netif.c
    ├── pbuf.c
    ├── raw.c
    ├── stats.c
    ├── sys.c
    ├── tcp.c
    ├── tcp_in.c
    ├── tcp_out.c
    ├── timeouts.c
    ├── udp.c
    └── ipv4/
        ├── autoip.c
        ├── dhcp.c
        ├── etharp.c
        ├── icmp.c
        ├── igmp.c
        ├── ip4.c
        ├── ip4_addr.c
        └── ip4_frag.c
```

**lwIP Netif Files:**
```
lwip/
└── netif/
    └── ethernet.c
```

**lwIP API Files (if using netconn API):**
```
lwip/
└── api/
    ├── api_lib.c
    ├── api_msg.c
    ├── err.c
    ├── netbuf.c
    ├── netdb.c
    ├── netifapi.c
    ├── sockets.c
    └── tcpip.c
```

**lwIP Port Files:**
```
lwip/port/
├── sys_arch.c
└── netif/
    └── tc375_eth.c
```

**FreeRTOS Core Files:**
```
FreeRTOS/FreeRTOS-Kernel/
├── tasks.c
├── queue.c
├── list.c
├── timers.c
├── event_groups.c
├── stream_buffer.c
└── portable/
    ├── MemMang/heap_4.c
    └── GCC/TriCore_1782/
        ├── port.c
        └── porttrap.c
```

---

## 🔨 **Step 3: Compiler Defines**

Add these preprocessor defines:

```
Right-click project → Properties
→ C/C++ Build → Settings
→ TASKING C/C++ Compiler → Symbols
```

**Add:**
```
FREERTOS
LWIP_TIMEVAL_PRIVATE=0
__TC37XX__
```

---

## 🔗 **Step 4: Linker Configuration**

### 4.1 Check Linker Script

Open `Lcf_Tasking_Tricore_Tc.lsl` and verify:

```c
/* Ensure sufficient heap for FreeRTOS */
__HEAP_SIZE = 64K;  /* Increase if needed */

/* Context Save Area for TriCore */
__CSA_SIZE = 64K;   /* FreeRTOS needs CSA */
```

### 4.2 Memory Sections

Ensure these sections exist:
- `.text` (code)
- `.data` (initialized data)
- `.bss` (uninitialized data)
- `.heap` (dynamic allocation)
- `.csa` (TriCore context save area)

---

## ⚙️ **Step 5: Compiler Optimization**

### 5.1 For Debug Build

```
Properties → C/C++ Build → Settings
→ TASKING C/C++ Compiler → Optimization
```

**Set:**
- Optimization level: `-O0` (no optimization)
- Debug info: `-g` (full debug info)
- Trade-off: `4` (prefer speed over size)

### 5.2 For Release Build

**Set:**
- Optimization level: `-O2` (optimize for speed)
- Debug info: `-g` (keep some debug info)
- Trade-off: `2` (balance speed/size)

---

## 🧪 **Step 6: Verify Build**

### 6.1 Clean and Build

```
Project → Clean...
Project → Build Project
```

### 6.2 Expected Output

```
Building file: ../Cpu0_Main.c
Building file: ../lwip/port/sys_arch.c
Building file: ../lwip/port/netif/tc375_eth.c
Building file: ../FreeRTOS/FreeRTOS-Kernel/tasks.c
...
Linking...
Creating hex file...
Build Finished. 0 errors, 0 warnings.
```

### 6.3 Check Binary Size

```
Memory Region        Used      Total
────────────────────────────────────
PFLASH (code)        ~200 KB   6 MB
DSPR (data)          ~80 KB    512 KB
CSA (context)        64 KB     -
```

---

## ❗ **Common Build Errors**

### Error 1: "FreeRTOS.h: No such file"
**Solution:** Add FreeRTOS include paths (see Step 1)

### Error 2: "undefined reference to `vTaskStartScheduler`"
**Solution:** Add FreeRTOS source files (see Step 2)

### Error 3: "lwip/opt.h: No such file"
**Solution:** Add lwIP include paths (see Step 1)

### Error 4: "multiple definition of `__malloc_lock`"
**Solution:** Use FreeRTOS heap instead of system malloc

### Error 5: CSA starvation trap
**Solution:** Increase `__CSA_SIZE` in linker script

---

## 🚀 **Step 7: Flash and Debug**

### 7.1 Flash to Target

```
Right-click project
→ Debug As → AURIX C/C++ Application
```

### 7.2 Verify FreeRTOS Running

Set breakpoint in:
```c
void vTaskStartScheduler(void)  /* Should be called */
void vLEDTask(void *pvParameters)  /* Should run periodically */
```

### 7.3 Monitor via UART (Optional)

If UART debug is enabled:
```
Open serial terminal (115200 baud, 8N1)
You should see:
"FreeRTOS started"
"lwIP initialized"
"Ethernet link up"
```

---

## 📊 **Build Configuration Summary**

| Component | Files | Include Paths | Size Estimate |
|-----------|-------|---------------|---------------|
| **lwIP** | ~45 .c files | 4 paths | ~50-70 KB |
| **FreeRTOS** | ~10 .c files | 3 paths | ~15-20 KB |
| **GETH Driver** | iLLD built-in | iLLD | ~10 KB |
| **Application** | Cpu0_Main.c, etc. | - | ~5-10 KB |
| **Total** | ~60 files | 7-8 paths | **~100-150 KB** |

---

## 🔄 **Quick Setup Script (Advanced)**

If you want to automate include path setup, you can use Eclipse headless build:

```bash
# Not implemented yet - manual setup recommended for now
```

---

## ✅ **Verification Checklist**

- [ ] All include paths added (7-8 paths)
- [ ] lwIP source files added (~45 files)
- [ ] FreeRTOS source files added (~10 files)
- [ ] Port layer files added (sys_arch.c, tc375_eth.c)
- [ ] Compiler defines added (FREERTOS, etc.)
- [ ] Linker script verified (heap, CSA)
- [ ] Project builds without errors
- [ ] Binary size is reasonable (~100-150 KB)
- [ ] Ready to flash and debug

---

## 📖 **Next Steps**

After successful build:

1. **Test FreeRTOS**: Flash and verify LED blinks
2. **Initialize lwIP**: Add network initialization in main
3. **Test Ethernet**: Ping TC375 from PC
4. **Implement DoIP**: Add DoIP server task
5. **Integrate OTA**: Add OTA manager task

---

## 🆘 **Need Help?**

Check documentation:
- `lwip/README.md` - lwIP integration guide
- `FreeRTOS/README.md` - FreeRTOS setup guide
- `PROTOTYPE_SKETCH.md` - Overall design document

---

**Last Updated:** 2025-11-02  
**Tested On:** AURIX Development Studio 1.10.24, TC375 Lite Kit

