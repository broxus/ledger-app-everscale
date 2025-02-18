#ifndef __SIGN_H__
#define __SIGN_H__

#include <stdint.h>

/**
 * Handles the sign command
 * @param dataBuffer input data buffer
 * @param dataLength length of input data
 * @param flags pointer to flags
 */
void handleSign(uint8_t* dataBuffer,
               uint16_t dataLength,
               volatile unsigned int* flags);

#endif // __SIGN_H__
