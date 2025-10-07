#ifndef FLASH_UPDATE_H
#define FLASH_UPDATE_H

// includes
#include "stm32u5xx_hal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * SET FILTER ID IN THE C FILE
 * @param FILTER_ID1: Incoming CAN frame filter SRC -> Bootloader
 * @param FILTER_ID2: Outgoing CAN frame for debug Bootloader -> SRC
 */
extern const uint32_t FILTER_ID1;
extern const uint32_t FILTER_ID2;

/**
 * Parses the incoming CAN data into flash and verifies program integrity.
 * If program data is valid, triggers reset and bootloading sequence.
 * @param uint8_t *RxData1: Incoming CAN data message
 * @param uint32_t Data_Length: length of incoming CAN message
 */
void Flash_Parse(uint8_t *RxData1, uint32_t Data_Length);

/**
 * Creates a checksum for a given set of data.
 * @returns uint32_t: 4 byte hash representing the checksum
 * @param uint32_t *data: pointer to the data to hash
 * @param size_t len: length of data
 */
uint32_t crc32(uint8_t *data, size_t len);

/**
 * Callback for CAN Rx data with filter for FILTER_ID1. Calls Flash_Parse.
 * @Overrides HAL_FDCAN_RxFifo0Callback in can.h
 * @param FDCAN_HandleTypeDef *hfdcan: handle for the incoming can frame. Set in
 * CAN_INIT
 * @param uint32_t RxFifo0ITs: Default, see can library for details
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan,
                               uint32_t RxFifo0ITs);

#endif
