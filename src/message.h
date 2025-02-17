#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdint.h>

#define DEFAULT_WORKCHAIN_ID 0

struct ByteStream_t;
int prepare_to_sign(struct ByteStream_t* src,
                    uint8_t wc,
                    uint8_t* address,
                    uint8_t* prepend_address);

#endif
