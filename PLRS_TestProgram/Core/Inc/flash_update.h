#ifndef FLASH_UPDATE_H
#define FLASH_UPDATE_H

#include "stm32u5xx_hal.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

extern const uint32_t FILTER_ID1;
extern const uint32_t FILTER_ID2;

//length in bytes
void Flash_Parse(uint8_t* RxData1, uint32_t Data_Length);



uint32_t crc32(uint8_t *data, size_t len);
//override the default implementations
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);



#endif
