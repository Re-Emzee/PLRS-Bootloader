
#ifndef FLASH_UTIL_H
#define FLASH_UTIL_H

#include "stm32u5xx_hal.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define FDCAN_CHUNK_SIZE 64
#define APP_DEFAULT_ADD  0x08100000  //Start of bank 2 data. This is the start of the running application
#define APP_LOAD_ADD  0x08020000	//Start of bank 1 data
#define APP_MAX_SIZE 0xE0000
#define PAYLOAD_SIZE FDCAN_DLC_BYTES_64
#define CRC_SIZE FDCAN_DLC_BYTES_4
#define BOOT_FLAG_ADD 0x0801E000
#define BOOT_FLAG 0xDEADBEEF

void Set_Boot_Flag(bool boot);

/**
 * @brief  Erases flash pages on Flash Bank 2.
 *         This function uses the HAL FLASH routines to erase 128 pages starting
 * at Page 1.
 */
void Flash_Erase(int start_page, int num_pages, int bank);

/**
 * @brief  Writes data to flash memory in 16-byte (quadword) chunks.
 * @param  address: The starting flash memory address.
 * @param  data: Pointer to the data to be written.
 * @param  length: Length of the data in bytes.
 */
void Flash_Write(uint32_t address, uint8_t *data, uint32_t length);

/**
 * @brief  Testing functions, leave out for deployment
 */
void Simulate_FDCAN_Receive(const uint8_t *data, uint32_t data_len);

#endif
