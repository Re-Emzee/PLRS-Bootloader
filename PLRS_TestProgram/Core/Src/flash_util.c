#include "flash_util.h"
#include "main.h"

void Set_Boot_Flag(bool boot) {
    Flash_Erase(15, 1, 1);
    uint32_t value = boot ? BOOT_FLAG : 0xAAAAAAAA;
    Flash_Write(BOOT_FLAG_ADD, (uint8_t *)&value, sizeof(value));
}

/**
 * Clears The Application Bank, page index starts at 1
 */
void Flash_Erase(int start_page, int num_pages, int bank) {
  // Unlock flash before starting erase
  if (HAL_FLASH_Unlock() != HAL_OK) {
    // Handle flash unlock error
    return;
  }

  FLASH_EraseInitTypeDef eraseConfig;
  uint32_t sectorError = 0;

  eraseConfig.TypeErase = FLASH_TYPEERASE_PAGES;
  switch(bank){
  case 1:
	  eraseConfig.Banks = FLASH_BANK_1;
	  break;
  case 2:
	  eraseConfig.Banks = FLASH_BANK_2;
  }

  eraseConfig.Page = start_page;             // Check if page numbering starts at 0 or 1.
  eraseConfig.NbPages = num_pages; // Ensure this value matches your flash layout.

  HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&eraseConfig, &sectorError);
  if (status != HAL_OK) {
    // Optionally get the error code for debugging.
    //uint32_t error = HAL_FLASH_GetError();
    //printf("Flash erase error: 0x%08lX\r\n", (unsigned long)error);
    // Handle error (for example, log the error code)
    HAL_FLASH_Lock();
    return;
  }

  HAL_FLASH_Lock(); // Lock flash after erase
}

void Flash_Write(uint32_t address, uint8_t *data, uint32_t length) {
  HAL_FLASH_Unlock(); // Unlock Flash
  // Example: Writing data in 16-byte (quadword) chunks
  for (uint32_t i = 0; i < length; i += 16) {
    // Prepare a 16-byte (quadword) buffer
    uint8_t quadBuf[16];
    // Fill with 0xFF (or 0x00) to avoid uninitialized bytes
    memset(quadBuf, 0xFF, sizeof(quadBuf));

    // Copy up to 16 bytes from 'data' into quadBuf
    uint32_t chunkSize = (length - i) < 16 ? (length - i) : 16;
    memcpy(quadBuf, &data[i], chunkSize);

    // Program this 16-byte chunk
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, address + i,
                          (uint32_t)quadBuf) != HAL_OK) {
      HAL_FLASH_Lock();
      return; // Errorhandling
    }
  }

  HAL_FLASH_Lock(); // Lock Flash
}

//TESTING

void Simulate_FDCAN_Receive(const uint8_t *data, uint32_t data_len) {
	Flash_Erase(16, 112, 1);
		uint32_t flashAddress = APP_LOAD_ADD;
		uint32_t totalBytes = data_len;
		uint32_t offset = 0;
		uint8_t fdcanBuffer[FDCAN_CHUNK_SIZE];

		while (offset < totalBytes) {
			uint32_t bytesToTransfer = (totalBytes - offset) < FDCAN_CHUNK_SIZE ? (totalBytes - offset) : FDCAN_CHUNK_SIZE;
			memcpy(fdcanBuffer, &data[offset], bytesToTransfer);
			Flash_Write(flashAddress, fdcanBuffer, bytesToTransfer);

			flashAddress += bytesToTransfer;
			offset += bytesToTransfer;
	}

	Flash_Erase(1,128,2); //wipe bank2 before writing to avoid artifacts
	Flash_Write(APP_DEFAULT_ADD, (uint8_t *)APP_LOAD_ADD, APP_MAX_SIZE);
}

