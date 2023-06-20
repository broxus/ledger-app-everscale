#ifndef _BYTE_STREAM_H_
#define _BYTE_STREAM_H_

#include <stdint.h>

typedef struct ByteStream_t {
    uint16_t data_size;
    uint16_t offset;
    uint8_t* data;
} ByteStream_t;

void ByteStream_init(struct ByteStream_t* self, uint8_t* data, uint16_t data_size);
uint8_t* ByteStream_read_data(struct ByteStream_t* self, uint32_t data_size);
uint8_t ByteStream_read_byte(struct ByteStream_t* self);
uint32_t ByteStream_read_u32(struct ByteStream_t* self);
uint8_t* ByteStream_get_cursor(struct ByteStream_t* self);
uint16_t ByteStream_get_length(struct ByteStream_t* self);
uint64_t ByteStream_read_uint(struct ByteStream_t* self, uint16_t bytes);

#endif
