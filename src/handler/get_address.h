#pragma once

#include <stdint.h>

/**
 * Handles the get address command
 * @param cdata command data
 * @param display display flag
 * @param flags flags
 * @return 0 on success, error code on failure
 */
int handleGetAddress(buffer_t* cdata, bool display, volatile unsigned int* flags);
