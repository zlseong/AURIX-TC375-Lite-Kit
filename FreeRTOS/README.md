# FreeRTOS Integration for TC375 Zonal Gateway

## Overview
This directory contains FreeRTOS Real-Time Operating System integration for TC375 Lite Kit.

## Directory Structure
```
FreeRTOS/
├── FreeRTOS-Kernel/         (Downloaded FreeRTOS source)
│   ├── include/             (FreeRTOS headers)
│   ├── portable/
│   │   └── GCC/TriCore_1782/ (TriCore port - base for TC375)
│   ├── tasks.c
│   ├── queue.c
│   ├── list.c
│   ├── timers.c
│   └── event_groups.c
├── FreeRTOSConfig.h         (TC375-specific configuration)
└── README.md                (this file)
```

## FreeRTOS Configuration

### Hardware Settings
- **CPU Clock**: 300 MHz
- **Tick Rate**: 1000 Hz (1ms tick)
- **Heap Size**: 50 KB
- **Max Priorities**: 16 levels (0-15)

### Memory Usage
- **Heap**: 50 KB (dynamic allocation)
- **Stack per task**: 
  - Minimal: 512 bytes (128 words)
  - lwIP task: 4 KB
  - Application tasks: 1-2 KB each
- **Total estimated**: ~70-80 KB RAM

### Task Priorities
| Priority | Level | Tasks |
|----------|-------|-------|
| Idle | 0 | Idle task (automatic) |
| Low | 1 | LED blink, heartbeat |
| Normal | 5 | DoIP, JSON, OTA tasks |
| High | 10 | lwIP stack |
| Real-time | 14 | Critical tasks (future) |
| Timer | 15 | Software timer service |

## Building with AURIX Development Studio

### 1. Add Include Paths

In Eclipse project properties → C/C++ Build → Settings → Tool Settings → Compiler → Include Paths, add:

```
${workspace_loc:/${ProjName}/FreeRTOS/FreeRTOS-Kernel/include}
${workspace_loc:/${ProjName}/FreeRTOS/FreeRTOS-Kernel/portable/GCC/TriCore_1782}
${workspace_loc:/${ProjName}/FreeRTOS}
```

### 2. Add Source Files

Add these source files to your build:

**FreeRTOS Core:**
- `FreeRTOS/FreeRTOS-Kernel/tasks.c`
- `FreeRTOS/FreeRTOS-Kernel/queue.c`
- `FreeRTOS/FreeRTOS-Kernel/list.c`
- `FreeRTOS/FreeRTOS-Kernel/timers.c`
- `FreeRTOS/FreeRTOS-Kernel/event_groups.c`
- `FreeRTOS/FreeRTOS-Kernel/stream_buffer.c`

**Memory Management (choose one):**
- `FreeRTOS/FreeRTOS-Kernel/portable/MemMang/heap_4.c` (recommended for TC375)

**TriCore Port:**
- `FreeRTOS/FreeRTOS-Kernel/portable/GCC/TriCore_1782/port.c`
- `FreeRTOS/FreeRTOS-Kernel/portable/GCC/TriCore_1782/porttrap.c`

### 3. Linker Configuration

Ensure your linker script (`.lsl`) allocates sufficient memory:

```
/* CSA (Context Save Area) for TriCore context switching */
/* Allocate at least 64 KB for CSA */

/* Heap for FreeRTOS (50 KB + margin) */
/* Ensure at least 60 KB available in RAM */
```

### 4. Compiler Flags

Add these compiler flags:
- `-DFREERTOS` (define FreeRTOS is used)
- `-I<path-to-FreeRTOS>` (include paths)

## TriCore-Specific Considerations

### Context Save Area (CSA)
TriCore uses CSA instead of traditional stack for context switching:
- FreeRTOS allocates CSA dynamically
- Ensure sufficient CSA in linker script (64 KB recommended)
- Stack overflow detection is NOT available (CSA starvation instead)

### Interrupts
- **ISR Priority Range**: 1-255
- **FreeRTOS API Priority**: Above 10
- **Kernel Priority**: 20
- Use `portYIELD_FROM_ISR()` to yield from ISR

### Float Point Unit (FPU)
- TC375 has FPU enabled (`configENABLE_FPU = 1`)
- FPU context is automatically saved/restored

## Integration with lwIP

FreeRTOS provides OS abstraction for lwIP:
- Semaphores → `sys_sem_t`
- Mutexes → `sys_mutex_t`
- Mailboxes → `sys_mbox_t` (FreeRTOS queues)
- Threads → `sys_thread_t` (FreeRTOS tasks)

See `lwip/port/sys_arch.c` for implementation.

## Example Usage

### Creating a Task
```c
#include "FreeRTOS.h"
#include "task.h"

void vMyTask(void *pvParameters)
{
    for(;;)
    {
        /* Task code here */
        vTaskDelay(pdMS_TO_TICKS(100));  /* Delay 100ms */
    }
}

/* In main() */
xTaskCreate(
    vMyTask,                    /* Task function */
    "MyTask",                   /* Task name */
    128,                        /* Stack size (words) */
    NULL,                       /* Parameters */
    PRIORITY_NORMAL,            /* Priority */
    NULL                        /* Task handle */
);
```

### Starting the Scheduler
```c
/* In core0_main() after all initialization */
vTaskStartScheduler();  /* Never returns */

/* If we get here, there was insufficient heap */
for(;;);
```

## Debugging

### Enable Debug Features
In `FreeRTOSConfig.h`:
```c
#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    1
#define configGENERATE_RUN_TIME_STATS           1  /* Enable runtime stats */
```

### Get Task Info
```c
/* Get task stack high water mark (free space) */
UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);

/* Get task state */
eTaskState eState = eTaskGetState(xTaskHandle);
```

### Common Issues

**Problem**: Scheduler doesn't start
- **Solution**: Check heap size, ensure sufficient RAM

**Problem**: Task crashes immediately
- **Solution**: Increase task stack size

**Problem**: System becomes unresponsive
- **Solution**: Check for priority inversion, use mutexes correctly

**Problem**: CSA starvation trap
- **Solution**: Increase CSA allocation in linker script

## Memory Configuration

### Heap Management (heap_4.c)
- **Allocation**: First-fit algorithm
- **Coalescing**: Adjacent free blocks are merged
- **Thread-safe**: Built-in protection
- **Best for**: General purpose embedded systems

### Alternative: heap_5.c
If you need memory from multiple RAM regions:
```c
/* Define memory regions */
HeapRegion_t xHeapRegions[] = {
    { ( uint8_t * ) 0x70000000, 0x10000 },  /* 64KB DSPR */
    { ( uint8_t * ) 0x50000000, 0x10000 },  /* 64KB PSPR */
    { NULL, 0 }
};

/* Initialize heap */
vPortDefineHeapRegions(xHeapRegions);
```

## Performance Tuning

### Tick Rate
- 1000 Hz (1ms): Good balance for automotive
- 100 Hz (10ms): Lower overhead, less precise timing
- 10000 Hz (0.1ms): High precision, higher overhead

### Context Switch Time
- TriCore context switch: ~100 CPU cycles (~0.33 μs @ 300MHz)
- Very fast compared to ARM Cortex (saves entire context in CSA)

### Interrupt Latency
- FreeRTOS disabled: <10 cycles
- FreeRTOS enabled: <20 cycles (minimal overhead)

## Testing

### Basic Test
```c
/* LED blink task */
void vLEDTask(void *pvParameters)
{
    for(;;)
    {
        /* Toggle LED */
        IfxPort_togglePin(LED1_PORT, LED1_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Create task in main() */
xTaskCreate(vLEDTask, "LED", 128, NULL, PRIORITY_LOW, NULL);
vTaskStartScheduler();
```

### Verify Scheduling
```c
/* Print task list (requires UART debug output) */
char pcWriteBuffer[512];
vTaskList(pcWriteBuffer);
/* Output: Task name, state, priority, stack high water mark */
```

## References

- FreeRTOS Official: https://www.freertos.org/
- TriCore Architecture: https://www.infineon.com/tc375
- AURIX Development Studio: https://www.infineon.com/ads
- FreeRTOS Book: https://www.freertos.org/Documentation/RTOS_book.html

## License

FreeRTOS: MIT License (permissive, commercial-friendly)

