#include "byte_stream.h"
#include "errors.h"
#include "utils.h"

void ByteStream_init(struct ByteStream_t* self, uint8_t* data, uint16_t data_size) {
    VALIDATE(self && data && data_size, ERR_INVALID_DATA);
    self->data_size = data_size;
    self->offset = 0;
    self->data = data;
}

void ByteStream_move_by(struct ByteStream_t* self, uint16_t data_size) {
    VALIDATE(data_size < self->data_size - self->offset, ERR_END_OF_STREAM);
    self->offset += data_size;
}

void ByteStream_move_by_unsafe(struct ByteStream_t* self, uint16_t data_size) {
    VALIDATE(data_size <= self->data_size - self->offset, ERR_END_OF_STREAM);
    self->offset += data_size;
}

uint8_t* ByteStream_read_data(struct ByteStream_t* self, uint32_t data_size) {
    uint8_t* data = self->data + self->offset;
    ByteStream_move_by_unsafe(self, data_size);
    return data;
}

uint8_t ByteStream_read_byte(struct ByteStream_t* self) {
    uint8_t byte = self->data[self->offset];
    ByteStream_move_by(self, 1);
    return byte;
}

uint32_t ByteStream_read_u32(struct ByteStream_t* self) {
    uint32_t u32 = readUint32BE(self->data + self->offset);
    ByteStream_move_by(self, sizeof(uint32_t));
    return u32;
}

uint8_t* ByteStream_get_cursor(struct ByteStream_t* self) {
    return self->data + self->offset;
}

uint16_t ByteStream_get_length(struct ByteStream_t* self) {
    return self->data_size - self->offset;
}

uint64_t ByteStream_read_uint(struct ByteStream_t* self, uint16_t bytes) {
    VALIDATE(bytes > 0 && bytes <= 8, ERR_INVALID_DATA);

    uint64_t val;

    if (bytes == 1) {
        val = self->data[self->offset];
    } else if (bytes == 2) {
        val = readUint16BE(self->data + self->offset);
    } else if (bytes >= 3 && bytes <= 4) {
        val = readUint32BE(self->data + self->offset);
    } else if (bytes >= 5 && bytes <= 8) {
        val = readUint64BE(self->data + self->offset);
    } else {
        THROW(ERR_INVALID_DATA);
    }

    ByteStream_move_by(self, bytes);

    return val;
}
