#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdint.h>

#define DEFAULT_WORKCHAIN 0

struct ByteStream_t;
int prepare_to_sign(struct ByteStream_t* src);

#endif
