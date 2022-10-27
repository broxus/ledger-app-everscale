#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdint.h>

#define DEFAULT_WORKCHAIN 0
#define DEFAULT_ATTACHED_AMOUNT 500000000

struct ByteStream_t;
void prepare_to_sign(struct ByteStream_t* src);

#endif
