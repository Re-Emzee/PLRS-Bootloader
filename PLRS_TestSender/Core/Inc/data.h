#ifndef BOOTLOADER_DATA_H
#define BOOTLOADER_DATA_H

#include <stdint.h>

/* Extern declaration of the bootloader data array and its length.
 * These variables are defined in bootloader_data.c (generated from your .bin file).
 */
extern const uint8_t data[];
extern const unsigned int data_len;

#endif /* BOOTLOADER_DATA_H */
