#ifndef _UTILS_H_
#define _UTILS_H_

#include "os.h"
#include "globals.h"

#include <stdint.h>
#include <stdbool.h>

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION

#include "cx.h"

int get_public_key(uint32_t accountNumber, uint8_t* publicKeyArray);
int get_private_key(uint32_t accountNumber, cx_ecfp_private_key_t* privateKey);

#endif

void writeUint32BE(uint32_t val, uint8_t* bytes);
void writeUint64BE(uint64_t val, uint8_t* bytes);

uint16_t readUint16BE(uint8_t* buffer);
uint32_t readUint32BE(uint8_t* buffer);
uint64_t readUint64BE(uint8_t* buffer);
uint8_t leading_zeros(uint16_t value);

uint8_t convert_hex_amount_to_displayable(const uint8_t* amount,
                                          uint8_t decimals,
                                          uint8_t amount_length,
                                          char* out);

#define VALIDATE(cond, error)                                           \
    do {                                                                \
        if (!(cond)) {                                                  \
            PRINTF("Validation Error in %s: %d\n", __FILE__, __LINE__); \
            THROW(error);                                               \
        }                                                               \
    } while (0)

#define PRINT_LINE() PRINTF("Print line %s: %d\n", __FILE__, __LINE__)

#endif
