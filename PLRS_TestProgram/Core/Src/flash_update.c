/*
 * flash_update.c
 *
 *  Created on: Mar 22, 2025
 *      Author: Manu_, Emzee
 */
#include "flash_update.h"
#include "flash_util.h"
#include "can.h"
#include "main.h" //error handler defined here

//SET THE CAN IDs HERE
const uint32_t FILTER_ID1 =0x208;
const uint32_t FILTER_ID2 = 0x20a;

/* CAN Status flags */
typedef enum {
    CRC_OK         = 0xE0,
    CRC_ERROR      = 0xE1,
    INVALID_LENGTH = 0xE2,
    OVERFLOW       = 0xE3,
    UNKNOWN        = 0xEF
} CAN_STATUS;

extern FDCAN_HandleTypeDef hfdcan1;
extern uint32_t _bflag;
static uint32_t write_offset = 0; //indirectly tells us the size of the program, use padded crc
static bool receiving_firmware = false;

void Flash_Parse(uint8_t* RxData, uint32_t Data_Length) {
    if (!receiving_firmware) {
    	Flash_Erase(16, 112,1);
        write_offset = 0;
        receiving_firmware = true;
    }

    //We will use the length of the message to determine if it is a crc/end stub or data
    if (Data_Length == CRC_SIZE){
    	receiving_firmware = false;
    	uint32_t received_crc;
    	memcpy(&received_crc, RxData, CRC_SIZE);
    	uint32_t actual_crc = crc32((uint8_t*)APP_LOAD_ADD, write_offset);

    	if ( actual_crc == received_crc) {
    	    Set_Boot_Flag(true);
    	    CAN_Transmit(FILTER_ID2, FDCAN_STANDARD_ID, FDCAN_DLC_BYTES_1, (uint8_t[]){CRC_OK}, &hfdcan1);
    	    //HAL_Delay(100);
    	    uint32_t putIndexMask = hfdcan1.LatestTxFifoQRequest;
    	    while ((hfdcan1.Instance->TXBTO & putIndexMask) == 0U) {
    	    	//wait or timeout
    	    }
    	    NVIC_SystemReset();
    	} else {
    	    Set_Boot_Flag(false);
    	    Flash_Erase(16, 112,1);
    	    CAN_Transmit(FILTER_ID2, FDCAN_STANDARD_ID, FDCAN_DLC_BYTES_1, (uint8_t[]){CRC_ERROR}, &hfdcan1);
    	}
    }
    else if(Data_Length == (PAYLOAD_SIZE)){
    	if (write_offset + 64 < APP_MAX_SIZE) {
    	        Flash_Write(APP_LOAD_ADD + write_offset, RxData, 64);
    	        write_offset += 64;
    	    } else {
    	    	CAN_Transmit(FILTER_ID2, FDCAN_STANDARD_ID, FDCAN_DLC_BYTES_1, (uint8_t[]){OVERFLOW}, &hfdcan1);
    	    }
    }
    else {
    	CAN_Transmit(FILTER_ID2, FDCAN_STANDARD_ID, FDCAN_DLC_BYTES_1, (uint8_t[]){INVALID_LENGTH}, &hfdcan1);
        return;
    }
}

uint32_t crc32(uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;

    // Calculate number of bytes to pad so total length is a multiple of 64
    size_t padded_len = len;
    size_t rem = len % 64;
    if (rem != 0) {
        padded_len += (64 - rem);
    }

    for (size_t i = 0; i < padded_len; i++) {
        uint8_t byte;
        if (i < len) {
            byte = data[i];
        } else {
            byte = 0xFF;  // Pad byte
        }

        crc ^= byte;
        for (uint8_t j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }

    return ~crc;
}


void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
        FDCAN_RxHeaderTypeDef RxHeader;
        memset(RxData1, 0, 64);
        if (RxData1 == NULL) {
            Error_Handler();
        }
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &RxHeader, RxData1) != HAL_OK) {
            Error_Handler();
//            HAL_GPIO_WritePin(GPIOG, GPIO_PIN_2, GPIO_PIN_SET);
        }
        //check actual length incoming against the buf len rather than stringcmp?
//        RxData1_BufferLength = dlc_to_bytes(RxHeader.DataLength);
//        RxData1_Identifier = RxHeader.Identifier;

        BSP_LED_Toggle(LED_GREEN);
        /* BOOTLOADER CAN*/
		if (RxHeader.Identifier == FILTER_ID1) {
			BSP_LED_Toggle(LED_RED);
			Flash_Parse(RxData1, RxHeader.DataLength);
		}
    }
    /* added for debug */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
}




