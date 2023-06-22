#include "cell.h"
#include "utils.h"

void Cell_init(struct Cell_t* self, uint8_t* cell_begin, uint16_t cell_length) {
    VALIDATE(self && cell_begin, ERR_CELL_IS_EMPTY);
    self->cell_begin = cell_begin;
    self->cell_length = cell_length;
}

uint8_t Cell_get_d1(const struct Cell_t* self) {
    VALIDATE(self && self->cell_begin, ERR_CELL_IS_EMPTY);
    VALIDATE(self->cell_length > 0, ERR_INVALID_DATA);
    return self->cell_begin[0];
}

uint8_t Cell_get_d2(const struct Cell_t* self) {
    VALIDATE(self && self->cell_begin, ERR_CELL_IS_EMPTY);
    VALIDATE(self->cell_length > 1, ERR_INVALID_DATA);
    return self->cell_begin[1];
}

uint8_t Cell_get_data_size(const struct Cell_t* self) {
    uint8_t d2 = Cell_get_d2(self);
    return (d2 >> 1) + (((d2 & 1) != 0) ? 1 : 0);
}

uint8_t* Cell_get_data(const struct Cell_t* self) {
    VALIDATE(self && self->cell_begin, ERR_CELL_IS_EMPTY);
    return self->cell_begin + CELL_DATA_OFFSET;
}

uint8_t* Cell_get_refs(const struct Cell_t* self, uint8_t* refs_count) {
    uint8_t d1 = Cell_get_d1(self);
    *refs_count = d1 & 7;
    uint8_t data_size = Cell_get_data_size(self);
    return self->cell_begin + CELL_DATA_OFFSET + data_size;
}

uint16_t Cell_bit_len(struct Cell_t* self) {
    uint8_t data_size = Cell_get_data_size(self);

    uint16_t bit_len = data_size * 8;
    if ((Cell_get_d2(self) & 0b1) == 0) {
        return bit_len;
    }

    uint8_t* data = Cell_get_data(self);
    for (int8_t i = data_size - 1; i >= 0; --i) {
        if (data[i] == 0) {
            bit_len -= 8;
        } else {
            uint8_t skip = 1;
            uint8_t mask = 1;
            while ((data[i] & mask) == 0) {
                skip += 1;
                mask <<= 1;
            }
            bit_len -= skip;
            break;
        }
    }
    return bit_len;
}

uint16_t deserialize_cell(struct Cell_t* cell, const uint8_t cell_index, const uint8_t cells_count) {
    uint8_t d1 = Cell_get_d1(cell);
    uint8_t level = d1 >> 5; // level
    uint8_t hashes = (d1 & 16) == 16; // with hashes
    uint8_t exotic = (d1 & 8) == 8; // exotic
    uint8_t rc = d1 & 7;	// refs count
    uint8_t absent = rc == 7 && hashes;
    uint8_t pruned = d1 == PRUNED_BRANCH_D1;
    UNUSED(level);

    VALIDATE(!hashes, ERR_INVALID_DATA);
    VALIDATE(!exotic || pruned, ERR_INVALID_DATA); // only ordinary or pruned cells are valid
    VALIDATE(rc <= MAX_REFERENCES_COUNT, ERR_INVALID_DATA);
    VALIDATE(!absent, ERR_INVALID_DATA);

    uint8_t data_size = Cell_get_data_size(cell);
    VALIDATE(!pruned || pruned && (data_size == PRUNED_BRANCH_DATA_SIZE), ERR_INVALID_DATA);

    uint8_t refs_count = 0;
    uint8_t* refs = Cell_get_refs(cell, &refs_count);
    for (uint8_t i = 0; i < refs_count; ++i) {
        VALIDATE(cell && cell->cell_length > CELL_DATA_OFFSET + data_size + i, ERR_INVALID_DATA);

        uint8_t ref = refs[i];
        VALIDATE(ref <= cells_count && ref > cell_index, ERR_INVALID_DATA);
    }

    return CELL_DATA_OFFSET + data_size + refs_count; // cell size
}

void calc_cell_hash(Cell_t* cell, const uint8_t cell_index) {
    BocContext_t* bc = &boc_context;

    uint8_t d1 = Cell_get_d1(cell);
    if (d1 == PRUNED_BRANCH_D1) {
        // handle pruned branch case
        // 1 byte - cell type
        // 1 byte - level mask
        // 32 bytes - hash
        // 2 bytes - cell depth (BE)
        uint8_t* cell_data = Cell_get_data(cell);
        memcpy(bc->hashes + cell_index * HASH_SIZE, cell_data + 2, HASH_SIZE);
        bc->cell_depth[cell_index] = *(cell_data + 2 + HASH_SIZE + 1);
        return;
    }

    uint8_t hash_buffer[266]; // d1(1) + d2(1) + data(128) + 4 * (depth(2) + hash(32))

    uint16_t hash_buffer_offset = 0;
    hash_buffer[0] = d1 & 0b00011111;
    hash_buffer[1] = Cell_get_d2(cell);
    hash_buffer_offset += 2;
    uint8_t data_size = Cell_get_data_size(cell);
    if (bc->public_key_cell_index && cell_index == bc->public_key_cell_index) {
        memcpy(hash_buffer + hash_buffer_offset, bc->public_key_cell_data, data_size);
    } else {
        memcpy(hash_buffer + hash_buffer_offset, Cell_get_data(cell), data_size);
    }
    hash_buffer_offset += data_size;

    uint8_t refs_count = 0;
    uint8_t* refs = Cell_get_refs(cell, &refs_count);
    VALIDATE(refs_count <= MAX_REFERENCES_COUNT, ERR_INVALID_DATA);
    for (uint8_t child = 0; child < refs_count; ++child) {
        uint8_t* depth = &bc->cell_depth[cell_index];
        VALIDATE(refs[child] < MAX_CONTRACT_CELLS_COUNT, ERR_INVALID_DATA);
        uint8_t child_depth = bc->cell_depth[refs[child]];
        *depth = (*depth > child_depth + 1) ? *depth : (child_depth + 1);
        uint8_t buf[2];
        buf[0] = 0;
        buf[1] = child_depth;
        memcpy(hash_buffer + hash_buffer_offset, buf, sizeof(buf));
        hash_buffer_offset += sizeof(buf);
    }

    for (uint8_t child = 0; child < refs_count; ++child) {
        uint8_t* cell_hash = bc->hashes + refs[child] * HASH_SIZE;
        memcpy(hash_buffer + hash_buffer_offset, cell_hash, HASH_SIZE);
        hash_buffer_offset += HASH_SIZE;
    }

    int result = cx_hash_sha256(hash_buffer, hash_buffer_offset, bc->hashes + cell_index * HASH_SIZE, HASH_SIZE);
    VALIDATE(result == HASH_SIZE, ERR_INVALID_HASH);
}
