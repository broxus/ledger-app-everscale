#pragma once

#include <stdint.h>

/**
 * Handles the get public key command
 * @param cdata command data
 * @param display display flag
 * @param flags flags
 */
int handleGetPublicKey(buffer_t* cdata, bool display, volatile unsigned int* flags);
