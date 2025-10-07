
#ifndef FLASH_UTIL_H
#define FLASH_UTIL_H

#include "stm32u5xx_hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FDCAN_CHUNK_SIZE 64
#define APP_DEFAULT_ADD                                                        \
  0x08100000 // Start of bank 2 data. This is the start of the running
             // application
#define APP_LOAD_ADD 0x08020000 // Start of bank 1 data
#define APP_MAX_SIZE 0xE0000
#define PAYLOAD_SIZE FDCAN_DLC_BYTES_64
#define CRC_SIZE FDCAN_DLC_BYTES_4
#define BOOT_FLAG_ADD 0x0801E000
#define BOOT_FLAG 0xDEADBEEF

void Set_Boot_Flag(bool boot);

/**
 * Erases a specified section of memory pages at a specified location.
 * @param int start_page: start_page of memory to erase. Indexed starting at 1.
 * @param int num_pages: nunmber of pages from @start_page to erase.
 * @param int bank: location in memory to wipe
 */
void Flash_Erase(int start_page, int num_pages, int bank);

/**
 * Writes data to flash memory in 16-byte (quadword) chunks.
 * @param  uint32_t address: The starting flash memory address.
 * @param  uint8_t data: Pointer to the data to be written.
 * @param  uint32_t length: Length of the data in bytes.
 */
void Flash_Write(uint32_t address, uint8_t *data, uint32_t length);

/**
 * @brief  Testing functions, leave out for deployment
 */
void Simulate_FDCAN_Receive(const uint8_t *data, uint32_t data_len);

#endif
