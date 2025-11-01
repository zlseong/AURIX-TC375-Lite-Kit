# lwIP Integration for TC375 Zonal Gateway

## Overview
This directory contains lwIP (Lightweight IP) stack integration for TC375 Lite Kit.

## Directory Structure
```
lwip/
├── lwip-2.1.3/          (lwIP source - download separately)
│   ├── src/
│   │   ├── core/        (IP, TCP, UDP, etc.)
│   │   ├── netif/       (Network interfaces)
│   │   └── api/         (Socket API)
│   └── doc/
├── port/                (TC375-specific porting layer)
│   ├── sys_arch.c       (OS abstraction - FreeRTOS)
│   ├── sys_arch.h
│   ├── lwipopts.h       (lwIP configuration)
│   └── netif/
│       ├── tc375_eth.c  (GETH + lwIP glue code)
│       └── tc375_eth.h
└── README.md            (this file)
```

## Download lwIP Source

### Option 1: Official Release (Recommended)
```bash
# Download lwIP 2.1.3 from official site
wget https://download.savannah.nongnu.org/releases/lwip/lwip-2.1.3.zip
unzip lwip-2.1.3.zip -d lwip/
```

### Option 2: Git Clone
```bash
cd lwip
git clone https://git.savannah.nongnu.org/git/lwip.git lwip-2.1.3
cd lwip-2.1.3
git checkout STABLE-2_1_3_RELEASE
```

### Option 3: Manual Download
Visit: https://savannah.nongnu.org/projects/lwip/
Download: lwip-2.1.3.zip
Extract to: `lwip/lwip-2.1.3/`

## Integration Status

- [ ] lwIP source downloaded
- [ ] Port layer implemented (sys_arch.c)
- [ ] GETH netif driver (tc375_eth.c)
- [ ] Configuration (lwipopts.h)
- [ ] FreeRTOS integration
- [ ] Basic TCP/IP test (Ping)
- [ ] TCP server/client test

## Memory Requirements

### Flash (ROM)
- lwIP core: ~40-60 KB
- Netif driver: ~5-10 KB
- Total: ~50-70 KB

### RAM
- TCP/IP buffers: ~20-30 KB (configurable)
- PCBs (Protocol Control Blocks): ~10-15 KB
- Total: ~30-50 KB (depends on configuration)

## Network Configuration

### Default Settings (lwipopts.h)
```c
IP Address:   192.168.1.10
Netmask:      255.255.255.0
Gateway:      192.168.1.1
MAC Address:  02:00:00:00:00:01 (configurable)
```

### Ports
- DoIP Server: 13400 (TCP/UDP)
- JSON Server: 8765 (TCP)
- HTTP (future): 80

## Building

1. Ensure lwIP source is in `lwip/lwip-2.1.3/`
2. Add include paths in Eclipse/ADS:
   - `lwip/lwip-2.1.3/src/include`
   - `lwip/port`
3. Build project

## Testing

### Step 1: Ping Test
```bash
ping 192.168.1.10
```

### Step 2: TCP Echo Server
```bash
nc 192.168.1.10 7
```

### Step 3: DoIP Test
```bash
# Use DoIP tester tool
```

## References

- lwIP Official: https://savannah.nongnu.org/projects/lwip/
- lwIP Wiki: https://lwip.wikia.com/
- lwIP with FreeRTOS: https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/
- AURIX TC375: https://www.infineon.com/tc375

## License

lwIP: BSD License (permissive, commercial-friendly)

