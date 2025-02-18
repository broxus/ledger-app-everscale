#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * Handles the sign transaction command
 * @param dataBuffer input data buffer
 * @param dataLength length of input data
 * @param flags pointer to flags
 * @param is_first_chunk indicates if this is the first chunk of data
 * @param more indicates if more chunks are expected
 */
void handleSignTransaction(uint8_t* dataBuffer,
                         uint16_t dataLength,
                         volatile unsigned int* flags,
                         bool is_first_chunk,
                         bool more);
