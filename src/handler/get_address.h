#ifndef __GET_ADDRESS_H__
#define __GET_ADDRESS_H__

#include <stdint.h>

/**
 * Handles the get address command
 * @param p1 parameter 1 - confirmation flag (P1_CONFIRM or P1_NON_CONFIRM)
 * @param p2 parameter 2 (expected to be 0)
 * @param dataBuffer input data buffer
 * @param dataLength length of input data
 * @param flags pointer to flags
 * @param tx pointer to tx
 */
void handleGetAddress(uint8_t p1,
                     uint8_t p2,
                     uint8_t* dataBuffer,
                     uint16_t dataLength,
                     volatile unsigned int* flags,
                     volatile unsigned int* tx);

#endif // __GET_ADDRESS_H__
