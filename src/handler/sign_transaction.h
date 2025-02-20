#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * Handles the sign transaction command
 * @param cdata input data buffer
 * @param is_first_chunk indicates if this is the first chunk of data
 * @param more indicates if more chunks are expected
 * @param flags pointer to flags
 * @return 0 on success, -1 on error
 */
int handleSignTransaction(buffer_t* cdata,
                          bool is_first_chunk,
                          bool more,
                          volatile unsigned int* flags);
