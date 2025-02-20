#pragma once

#include <stdint.h>
#include "buffer.h"
/**
 * Handles the sign command
 * @param cdata input data buffer
 * @param flags pointer to flags
 */
int handleSign(buffer_t* cdata, volatile unsigned int* flags);
