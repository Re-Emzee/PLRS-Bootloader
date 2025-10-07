# Table of Contents

- [Overview](#overview)
- [Memory Partitions](#memory-partitions)
  - [Application Memory](#application-memory)
  - [Application Buffer](#application-buffer)
  - [Boot Flags](#boot-flags)
- [Bootloader Base Program](#bootloader-base-program)
- [Bootloader Utility Library](#bootloader-utility-library)
- [Configuration](#configuration)
  - [Changing the Boot Sequence](#changing-the-boot-sequence)
  - [Build Process](#build-process)
- [CAN Program Sender](#can-program-sender)
  - [File Responsibilities](#file-responsibilities)
  - [Transmission Format](#transmission-format)
  - [CAN ID and Filter Configuration](#can-id-and-filter-configuration)
- [Usage](#usage)
  - [Main Branch Base Program](#main-branch-base-program)
    - [Receiving Board](#receiving-board)
    - [Sending Board](#sending-board)
- [Testing](#testing)
  - [Test Program For Example Application](#test-program-for-example-application)
  - [Test Sender For Example Host](#test-sender-for-example-host)
  - [Test Setup And Demo](#test-setup-and-demo)
- [Debugging and Issues](#debugging-and-issues)
- [Areas for Improvement](#areas-for-improvement)
- [Development Notes](#development-notes)
- [Quick Links](#quick-links)

---

# Overview

“Would be better to not have to swim up to the boat to push an update.” — *Michael prob?*

This page houses the **PLRS Bootloading Project** responsible for remote bootloading capabilities for all STM32U5 boards across PLRS. Firmware can be re-flashed and updated by sending a program binary through the **CAN bus** without the need to physically connect to the board.

There are three components to the bootloading system:

1. **Bootloader Base Program**
2. **Bootloader Utility Library**
3. **CAN Program Sender**

> Note: The base program refers to the actual bootloading program that jumps to the application (the target running program sent over CAN).

---

# Memory Partitions

The STM32U5 features **two physical flash memory banks** (Bank 1 and Bank 2, per HAL).  
Each bank is **1 MB**, 8-byte aligned, and contains **128 pages of 8 KB**.  
For different boards, check the memory section in the `.ioc` file.

![Memory Layout](image-20250517-194532.png)

Due to memory integrity safeguards, programs cannot write to the bank they are currently running in.  
Thus:

- **Bank 1:** Bootloader base program (entry point)
- **Bank 2:** Application (running program)

Additional partitions handle CRC integrity and flags.

| Address | Bank | Section Size | Description | Config Macro |
|----------|------|---------------|--------------|---------------|
| `0x08200000` | 2 | — | Reserved | — |
| `0x08100000` | 2 | 1 MB | Application Memory | `APP_DEFAULT_ADD` |
| `0x08020000` | 1 | 916 KB (0xE0000) | Application Buffer | `APP_LOAD_ADD` |
| `0x0801E000` | 1 | 8 KB | Boot Flags | `BOOT_FLAG_ADD` |
| `0x08000000` | 1 | 120 KB (0x1E000) | Bootloader Base Program | `FLASH_ORIGIN` |

---

## Application Memory

This is where the actual **application program data** is stored.  
Entry point: `0x08020000`, configurable via `APP_DEFAULT_ADD` in `flash_update.h`.

---

## Application Buffer

Incoming CAN program data is stored here, starting at `0x08020000`.  
A CRC integrity check is performed before copying to Bank 2.  
This section is **physical flash**, not RAM.

---

## Boot Flags

One page prior to the buffer determines if CRC check passed (true = write to Bank 2).

---

# Bootloader Base Program

Main bootloader logic. Uses ~15 pages (120 KB) for testing/debugging.

```c
/* USER CODE BEGIN 1 */
while (1) { 
  if ((*(uint32_t*)BOOT_FLAG_ADD) == BOOT_FLAG) {
    Flash_Erase(1, 128, 2);
    memcpy((uint32_t *)APP_DEFAULT_ADD, (uint32_t *)APP_LOAD_ADD, APP_MAX_SIZE);
    Set_Boot_Flag(false);
  } else {
    uint32_t sp = *(__IO uint32_t*) APP_DEFAULT_ADD;
    if (sp >= MEM_LOWER_BOUND && sp < MEM_UPPER_BOUND) {
      __set_MSP(*(__IO uint32_t *)APP_DEFAULT_ADD);
      ((pFunction)(*(__IO uint32_t *)(APP_DEFAULT_ADD + 4)))();
    }
  }
}
/* USER CODE END 1 */
```

### Linker Script (`stm32U575ZITXQ_FLASH.ld`)
```ld
MEMORY
{
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 768K
  SRAM4 (xrw) : ORIGIN = 0x28000000, LENGTH = 16K
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 120K
}
```

### Summary of Functionality

1. Check boot flag:  
   - If **true**, erase Bank 2 → copy buffer → clear flag.  
   - If **false**, skip to application jump.
2. Set stack pointer and validate region.
3. Jump to application reset handler.
4. Receive CAN updates (64B messages + 4B CRC).
   - If CRC_OK → copy + reset.  
   - If CRC_ERROR → reset buffer.

---

# Bootloader Utility Library

Simplifies integration into application code. Includes:

| File | Function |
|------|-----------|
| `Core/Src/flash_util.c` | Flash write/erase utilities |
| `Core/Src/flash_update.c` | CAN callbacks, data parsing |
| `Core/Src/can.c` | FDCAN config, filter setup, and transmission |

---

# Configuration

### `flash_util.h`
```c
#define FDCAN_CHUNK_SIZE 64
#define APP_DEFAULT_ADD  0x08100000
#define APP_LOAD_ADD     0x08020000
#define APP_MAX_SIZE     0xE0000
#define PAYLOAD_SIZE     FDCAN_DLC_BYTES_64
#define CRC_SIZE         FDCAN_DLC_BYTES_4
#define BOOT_FLAG_ADD    0x0801E000
#define BOOT_FLAG        0xDEADBEEF
```

### CAN IDs (`flash_update.c`)
```c
const uint32_t FILTER_ID1 = 0x208;
const uint32_t FILTER_ID2 = 0x20A;
```

---

## Changing the Boot Sequence

Update vector and flash base address:

**`stm32u575xx.h`**
```c
#define FLASH_BASE 0x08100000
```

**`stm32u575xx.c`**
```c
#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM1_BASE | VECT_TAB_OFFSET;
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
#endif
```

**Linker:**
```ld
FLASH (rx) : ORIGIN = 0x08100000, LENGTH = 1024K
```

---

## Build Process

```bash
arm-none-eabi-objcopy -O binary BootloaderTestProgram.elf BootloaderTestProgram.bin
python3 bin2c.py BootloaderTestProgram.bin data.c data
```

---

# CAN Program Sender

Utility for flashing firmware over CAN.

### Responsibilities
| File | Function |
|------|-----------|
| `main.c` | Peripheral init, main CAN loop |
| `can.c` | FDCAN configuration |
| `data.c` | Auto-generated firmware data array |

### Transmission Process

1. Split firmware into 64-byte chunks.  
2. Send via `CAN_Transmit()` with defined IDs.  
3. CRC (4B) sent at end.

### CAN Filters
- **0x208:** Firmware chunk ID  
- **0x20A:** Secondary/acknowledge ID

---

# Usage

Clone the repo:
```bash
git clone --branch main --single-branch https://github.com/UBCSailbot/PLRS_Bootloading.git
```

## Receiving Board

1. Flash `PLRS_BOOTLOADER` project.  
2. Flash application afterward.  
3. System jumps automatically to app on reset.

## Sending Board

Use second STM32 or Raspberry Pi.  
Ensure 64B chunks and CRC appended at end.

---

# Testing

## Example Application

Blinks green LED; button toggles blue; red flashes on CAN receive.

## Example Host Sender

Sends compiled binary over CAN with CRC check.

```c
while (offset < data_len) {
  memset(TxData1, 0xFF, 64);
  memcpy(TxData1, &data[offset], chunk_size);
  CAN_Transmit(FILTER_ID1, FDCAN_STANDARD_ID, FDCAN_DLC_BYTES_64, TxData1, &hfdcan1);
  HAL_Delay(50);
  offset += chunk_size;
}
```

---

# Debugging and Issues

- Using `HAL_Delay()` may cause inconsistent timing. Replace with interrupt-based or polling mechanism.  
- Check flash writes in debug memory viewer.  
- FIFO overflow → consider longer delay or send ack system.

---

# Areas for Improvement

- Optimize `memcpy` calls.  
- Reduce bootloader binary size.  
- Implement non-blocking CAN transmit.  
