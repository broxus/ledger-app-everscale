#include "contract.h"
#include "slice_data.h"
#include "byte_stream.h"
#include "cell.h"
#include "utils.h"
#include "errors.h"
#include "hashmap_label.h"

// Wallets code
//
// Each array is a minimal BOC containing the wallet's StateInit cells
// with the code cell replaced by a pruned branch cell (d1=0x28, d2=0x48)
// that embeds the real code hash and depth. The public key cell holds a
// zero placeholder that compute_multisig_address fills in at runtime.
//
// Group A layout (safe_multisig, safe_multisig_24h, setcode_multisig):
//   7 cells: root(0) refs=[6,1] | data-root(1) | hashmap-mid(2) |
//            hashmap-left(3) | hashmap-leaf(4) | pubkey(5) | pruned-code(6)
//
// Group B layout (bridge_multisig, multisig_2, multisig_2_1, surf):
//   4 cells: root(0) refs=[3,1] | data-root(1) | pubkey(2) | pruned-code(3)

// -- Group A header: ref_size=1, offset_size=2, cells=7, roots=1, absent=0,
//                   total_cells_size=0x0060 (96 bytes), root_index=0
#define GROUP_A_HDR 0x01, 0x02, 0x07, 0x01, 0x00, 0x00, 0x60, 0x00
// -- Group A cells 0-5 (58 bytes) --
#define GROUP_A_CELLS_0_5                                                          \
    /* Cell 0 root:    d1=0x02, d2=0x01, data=[0x34], refs=[6,1] */                \
    0x02, 0x01, 0x34, 0x06, 0x01,                                                  \
    /* Cell 1 hashmap: d1=0x01, d2=0x01, data=[0xC0], refs=[2] */                  \
    0x01, 0x01, 0xC0, 0x02,                                                        \
    /* Cell 2 hashmap: d1=0x02, d2=0x03, data=[0xCF,0x20], refs=[5,3] */           \
    0x02, 0x03, 0xCF, 0x20, 0x05, 0x03,                                            \
    /* Cell 3 hashmap: d1=0x01, d2=0x01, data=[0xDE], refs=[4] */                  \
    0x01, 0x01, 0xDE, 0x04,                                                        \
    /* Cell 4 hashmap: d1=0x00, d2=0x03, data=[0xD0,0x20] */                       \
    0x00, 0x03, 0xD0, 0x20,                                                        \
    /* Cell 5 pubkey:  d1=0x00, d2=0x41, placeholder zeros */                      \
    0x00, 0x41, 0xD8,                                                              \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                      \
    0x04

// -- Group B header: cells=4, total_cells_size=0x0053 (83 bytes)
#define GROUP_B_HDR 0x01, 0x02, 0x04, 0x01, 0x00, 0x00, 0x53, 0x00
// -- Group B cells 0-2 (45 bytes) --
#define GROUP_B_CELLS_0_2                                                          \
    /* Cell 0 root:    d1=0x02, d2=0x01, data=[0x34], refs=[3,1] */                \
    0x02, 0x01, 0x34, 0x03, 0x01,                                                  \
    /* Cell 1 hashmap: d1=0x01, d2=0x01, data=[0xC0], refs=[2] */                  \
    0x01, 0x01, 0xC0, 0x02,                                                        \
    /* Cell 2 pubkey:  d1=0x00, d2=0x43, placeholder zeros */                      \
    0x00, 0x43, 0xD0,                                                              \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                \
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                \
    0x20

// Pruned branch cell (38 bytes): d1=0x28, d2=0x48, data=[type=1, lvl=1, hash(32), depth_hi, depth_lo]
const uint8_t safe_multisig_wallet[] = {
    GROUP_A_HDR,
    GROUP_A_CELLS_0_5,
    /* Cell 6: pruned code cell, depth=0x0C */
    0x28, 0x48, 0x01, 0x01,
    0x80, 0xd6, 0xc4, 0x7c, 0x4a, 0x25, 0x54, 0x3c, 0x9b, 0x39, 0x7b, 0x71, 0x71, 0x6f, 0x3f, 0xae,
    0x1e, 0x2c, 0x5d, 0x24, 0x71, 0x74, 0xc5, 0x2e, 0x2c, 0x19, 0xbd, 0x89, 0x64, 0x42, 0xb1, 0x05,
    0x00, 0x0C};

const uint8_t safe_multisig_wallet_24h[] = {
    GROUP_A_HDR,
    GROUP_A_CELLS_0_5,
    /* Cell 6: pruned code cell, depth=0x0C */
    0x28, 0x48, 0x01, 0x01,
    0x7d, 0x09, 0x96, 0x94, 0x34, 0x06, 0xf7, 0xd6, 0x2a, 0x4f, 0xf2, 0x91, 0xb1, 0x22, 0x8b, 0xf0,
    0x6e, 0xbd, 0x3e, 0x04, 0x8b, 0x58, 0x43, 0x6c, 0x5b, 0x70, 0xfb, 0x77, 0xff, 0x8b, 0x4b, 0xf2,
    0x00, 0x0C};

const uint8_t setcode_multisig_wallet[] = {
    GROUP_A_HDR,
    GROUP_A_CELLS_0_5,
    /* Cell 6: pruned code cell, depth=0x0D */
    0x28, 0x48, 0x01, 0x01,
    0xe2, 0xb6, 0x0b, 0x6b, 0x60, 0x2c, 0x10, 0xce, 0xd7, 0xea, 0x8e, 0xde, 0x4b, 0xdf, 0x96, 0x34,
    0x2c, 0x97, 0x57, 0x0a, 0x37, 0x98, 0x06, 0x6f, 0x3f, 0xb5, 0x0a, 0x4b, 0x2b, 0x27, 0xa2, 0x08,
    0x00, 0x0D};

const uint8_t bridge_multisig_wallet[] = {
    GROUP_B_HDR,
    GROUP_B_CELLS_0_2,
    /* Cell 3: pruned code cell, depth=0x0B */
    0x28, 0x48, 0x01, 0x01,
    0xF3, 0xA0, 0x7A, 0xE8, 0x4F, 0xC3, 0x43, 0x25, 0x9D, 0x7F, 0xA4, 0x84, 0x7B, 0x86, 0x33, 0x5B,
    0x3F, 0xDC, 0xFC, 0x8B, 0x31, 0xF1, 0xBA, 0x4B, 0x7A, 0x94, 0x99, 0xD5, 0x53, 0x0F, 0x0B, 0x18,
    0x00, 0x0B};

const uint8_t multisig_2_wallet[] = {
    GROUP_B_HDR,
    GROUP_B_CELLS_0_2,
    /* Cell 3: pruned code cell, depth=0x0C */
    0x28, 0x48, 0x01, 0x01,
    0x29, 0xb2, 0x47, 0x76, 0xb3, 0xdf, 0x6a, 0x05, 0xc5, 0xa1, 0xb8, 0xd8, 0xfd, 0x75, 0xcb, 0x72,
    0xa1, 0xd3, 0x3c, 0x0a, 0x44, 0x38, 0x53, 0x32, 0xa8, 0xbf, 0xc2, 0x72, 0x7f, 0xb6, 0x65, 0x90,
    0x00, 0x0C};

const uint8_t multisig_2_1_wallet[] = {
    GROUP_B_HDR,
    GROUP_B_CELLS_0_2,
    /* Cell 3: pruned code cell, depth=0x0A */
    0x28, 0x48, 0x01, 0x01,
    0xd6, 0x6d, 0x19, 0x87, 0x66, 0xab, 0xdb, 0xe1, 0x25, 0x3f, 0x34, 0x15, 0x82, 0x6c, 0x94, 0x6c,
    0x37, 0x1f, 0x51, 0x12, 0x55, 0x24, 0x08, 0x62, 0x5a, 0xeb, 0x0b, 0x31, 0xe0, 0xef, 0x2d, 0xf3,
    0x00, 0x0A};

const uint8_t surf_wallet[] = {
    GROUP_B_HDR,
    GROUP_B_CELLS_0_2,
    /* Cell 3: pruned code cell, depth=0x0C */
    0x28, 0x48, 0x01, 0x01,
    0x20, 0x7d, 0xc5, 0x60, 0xc5, 0x95, 0x6d, 0xe1, 0xa2, 0xc1, 0x47, 0x93, 0x56, 0xf8, 0xf3, 0xee,
    0x70, 0xa5, 0x97, 0x67, 0xdb, 0x2b, 0xf4, 0x78, 0x8b, 0x1d, 0x61, 0xad, 0x42, 0xcd, 0xad, 0x82,
    0x00, 0x0C};

// Wallets code hash
// Everscale
const uint8_t wallet_v3_code_hash[] = {
    0x84, 0xDA, 0xFA, 0x44, 0x9F, 0x98, 0xA6, 0x98, 0x77, 0x89, 0xBA, 0x23, 0x23, 0x58, 0x07, 0x2B,
    0xC0, 0xF7, 0x6D, 0xC4, 0x52, 0x40, 0x02, 0xA5, 0xD0, 0x91, 0x8B, 0x9A, 0x75, 0xD2, 0xD5, 0x99};

const uint8_t ever_wallet_code_hash[] = {
    0x3B, 0xA6, 0x52, 0x8A, 0xB2, 0x69, 0x4C, 0x11, 0x81, 0x80, 0xAA, 0x3B, 0xD1, 0x0D, 0xD1, 0x9F,
    0xF4, 0x00, 0xB9, 0x09, 0xAB, 0x4D, 0xCF, 0x58, 0xFC, 0x69, 0x92, 0x5B, 0x2C, 0x7B, 0x12, 0xA6};

// TON
const uint8_t wallet_v4r1_code_hash[] = {
    0x64, 0xDD, 0x54, 0x80, 0x55, 0x22, 0xC5, 0xBE, 0x8A, 0x9D, 0xB5, 0x9C, 0xEA, 0x01, 0x05, 0xCC,
    0xF0, 0xD0, 0x87, 0x86, 0xCA, 0x79, 0xBE, 0xB8, 0xCB, 0x79, 0xE8, 0x80, 0xA8, 0xD7, 0x32, 0x2D};

const uint8_t wallet_v4r2_code_hash[] = {
    0xFE, 0xB5, 0xFF, 0x68, 0x20, 0xE2, 0xFF, 0x0D, 0x94, 0x83, 0xE7, 0xE0, 0xD6, 0x2C, 0x81, 0x7D,
    0x84, 0x67, 0x89, 0xFB, 0x4A, 0xE5, 0x80, 0xC8, 0x78, 0x86, 0x6D, 0x95, 0x9D, 0xAB, 0xD5, 0xC0};

const uint8_t wallet_v3r1_code_hash[] = {
    0xB6, 0x10, 0x41, 0xA5, 0x8A, 0x79, 0x80, 0xB9, 0x46, 0xE8, 0xFB, 0x9E, 0x19, 0x8E, 0x3C, 0x90,
    0x4D, 0x24, 0x79, 0x9F, 0xFA, 0x36, 0x57, 0x4E, 0xA4, 0x25, 0x1C, 0x41, 0xA5, 0x66, 0xF5, 0x81};

// wallet_v3r2_code_hash == wallet_v3_code_hash (same code, different wallet_id vs Everscale)

const uint8_t wallet_v5r1_code_hash[] = {
    0x20, 0x83, 0x4b, 0x7b, 0x72, 0xb1, 0x12, 0x14, 0x7e, 0x1b, 0x2f, 0xb4, 0x57, 0xb8, 0x4e, 0x74,
    0xd1, 0xa3, 0x0f, 0x04, 0xf7, 0x37, 0xd4, 0xf6, 0x2a, 0x66, 0x8e, 0x95, 0x52, 0xd2, 0xb7, 0x2f};


void deserialize_cells_tree(struct ByteStream_t* src) {
    uint8_t first_byte = ByteStream_read_byte(src);
    {
        bool index_included = (first_byte & 0x80) != 0;
        bool has_crc = (first_byte & 0x40) != 0;
        bool has_cache_bits = (first_byte & 0x20) != 0;
        uint8_t flags = (first_byte & 0x18) >> 3;
        UNUSED(flags);
        VALIDATE(!index_included && !has_crc && !has_cache_bits, ERR_INVALID_CONTRACT);
    }

    uint8_t ref_size = first_byte & 0x7;  // size in bytes
    VALIDATE(ref_size == 1, ERR_INVALID_CONTRACT);

    uint8_t offset_size = ByteStream_read_byte(src);
    VALIDATE(offset_size != 0 && offset_size <= 8, ERR_INVALID_CONTRACT);

    uint8_t cells_count = ByteStream_read_uint(src, ref_size);

    uint8_t roots_count = ByteStream_read_uint(src, ref_size);
    VALIDATE(roots_count == MAX_ROOTS_COUNT, ERR_INVALID_CONTRACT);

    {
        uint8_t absent_count = ByteStream_read_uint(src, ref_size);
        UNUSED(absent_count);
        uint8_t* total_cells_size = ByteStream_read_data(src, offset_size);
        UNUSED(total_cells_size);
        uint8_t* buf = ByteStream_read_data(src, roots_count * ref_size);
        UNUSED(buf);
    }

    // Reset cells count and depth table
    boc_context.cells_count = 0;
    memset(boc_context.cell_depth, 0, sizeof(boc_context.cell_depth));

    Cell_t cell;
    for (uint8_t i = 0; i < cells_count; ++i) {
        VALIDATE(i < MAX_CONTRACT_CELLS_COUNT, ERR_INVALID_CONTRACT);

        uint8_t* cell_begin = ByteStream_get_cursor(src);
        uint16_t cell_length = ByteStream_get_length(src);
        Cell_init(&cell, cell_begin, cell_length);
        uint16_t offset = deserialize_cell(&cell, i, cells_count);
        boc_context.cells[i] = cell;
        ByteStream_read_data(src, offset);

        // Count only initialized cells
        boc_context.cells_count++;

        if (src->offset >= src->data_size) {
            break;
        }
    }
}

void find_public_key_cell() {
    BocContext_t* bc = &boc_context;
    VALIDATE(Cell_get_data(&bc->cells[0])[0] & 0x20, ERR_INVALID_CONTRACT);  // has data branch

    uint8_t refs_count = 0;
    uint8_t* refs = Cell_get_refs(&bc->cells[0], &refs_count);
    VALIDATE(refs_count > 0 && refs_count <= 2, ERR_INVALID_CONTRACT);

    uint8_t data_root = refs[refs_count - 1];
    VALIDATE(data_root != 0 && data_root <= MAX_CONTRACT_CELLS_COUNT, ERR_INVALID_CONTRACT);
    refs = Cell_get_refs(&bc->cells[data_root], &refs_count);
    VALIDATE(refs_count != 0 && refs_count <= MAX_REFERENCES_COUNT, ERR_INVALID_CONTRACT);

    uint8_t key_buffer[8];
    SliceData_t key;
    memset(key_buffer, 0, sizeof(key_buffer));
    SliceData_init(&key, key_buffer, sizeof(key_buffer));

    uint16_t bit_len = SliceData_remaining_bits(&key);
    put_to_node(refs[0], bit_len, &key);
}

#ifndef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION

void compute_wallet_v3_address(uint32_t account_number,
                               uint32_t wallet_id,
                               const uint8_t* code_hash,
                               uint8_t* address) {
    uint8_t data_hash[HASH_SIZE];

    // Data cell: seqno(32)=0 + wallet_id(32) + pubkey(256) = 320 bits
    // d1=0x00, d2=0x50, 40 data bytes (byte-aligned)
    {
        uint8_t hash_buffer[42];  // d1(1) + d2(1) + seqno+wallet_id(8) + pubkey(32)
        uint16_t hash_buffer_offset = 0;

        hash_buffer[0] = 0x00;  // d1
        hash_buffer[1] = 0x50;  // d2
        hash_buffer_offset += 2;

        // seqno(32)=0 as high 4 bytes, wallet_id(32) as low 4 bytes
        writeUint64BE(wallet_id, hash_buffer + hash_buffer_offset);
        hash_buffer_offset += sizeof(uint64_t);

        uint8_t public_key[PUBLIC_KEY_LENGTH];
        VALIDATE(get_public_key(account_number, public_key) == 0, ERR_GET_PUBLIC_KEY_FAILED);

        memcpy(hash_buffer + hash_buffer_offset, public_key, PUBLIC_KEY_LENGTH);
        hash_buffer_offset += PUBLIC_KEY_LENGTH;

        int result = cx_hash_sha256(hash_buffer, hash_buffer_offset, data_hash, HASH_SIZE);
        VALIDATE(result == HASH_SIZE, ERR_INVALID_HASH);
    }

    // StateInit repr_hash: SHA256(d1 | d2 | 0x34 | code_depth(2B) | data_depth(2B) | code_hash | data_hash)
    // code_depth=0, data_depth=0
    {
        uint8_t hash_buffer[71];  // d1(1) + d2(1) + data(1) + depths(4) + code_hash(32) + data_hash(32)

        uint16_t hash_buffer_offset = 0;
        hash_buffer[0] = 0x02;  // d1
        hash_buffer[1] = 0x01;  // d2
        hash_buffer_offset += 2;

        hash_buffer[2] = 0x34;  // StateInit data
        hash_buffer_offset += 1;

        writeUint32BE(0x00, hash_buffer + hash_buffer_offset);
        hash_buffer_offset += sizeof(uint32_t);

        memcpy(hash_buffer + hash_buffer_offset, code_hash, HASH_SIZE);
        hash_buffer_offset += HASH_SIZE;

        memcpy(hash_buffer + hash_buffer_offset, data_hash, HASH_SIZE);
        hash_buffer_offset += HASH_SIZE;

        int result = cx_hash_sha256(hash_buffer, hash_buffer_offset, address, HASH_SIZE);
        VALIDATE(result == HASH_SIZE, ERR_INVALID_HASH);
    }
}

void compute_ever_wallet_address(uint32_t account_number, uint8_t* address) {
    uint8_t data_hash[HASH_SIZE];

    // Compute data hash
    {
        uint8_t hash_buffer[42];  // d1(1) + d2(1) + pubkey(32) + data(8)

        uint16_t hash_buffer_offset = 0;

        hash_buffer[0] = 0x00;  // d1(1)
        hash_buffer[1] = 0x50;  // d2(1)
        hash_buffer_offset += 2;

        // Pubkey
        uint8_t public_key[PUBLIC_KEY_LENGTH];
        VALIDATE(get_public_key(account_number, public_key) == 0, ERR_GET_PUBLIC_KEY_FAILED);

        memcpy(hash_buffer + hash_buffer_offset, public_key, PUBLIC_KEY_LENGTH);
        hash_buffer_offset += PUBLIC_KEY_LENGTH;

        // Data
        writeUint64BE(0, hash_buffer + hash_buffer_offset);
        hash_buffer_offset += sizeof(uint64_t);

        // Calculate data hash
        int result = cx_hash_sha256(hash_buffer, hash_buffer_offset, data_hash, HASH_SIZE);
        VALIDATE(result == HASH_SIZE, ERR_INVALID_HASH);
    }

    // Compute address
    {
        uint8_t hash_buffer[71];  // d1(1) + d2(1) + data(5) + code_hash(32) + data_hash(32)

        uint16_t hash_buffer_offset = 0;
        hash_buffer[0] = 0x02;  // d1(1)
        hash_buffer[1] = 0x01;  // d2(1)
        hash_buffer_offset += 2;

        // Data
        hash_buffer[2] = 0x34;
        hash_buffer_offset += 1;

        writeUint32BE(0x30000, hash_buffer + hash_buffer_offset);
        hash_buffer_offset += sizeof(uint32_t);

        // Code hash
        memcpy(hash_buffer + hash_buffer_offset,
               ever_wallet_code_hash,
               sizeof(ever_wallet_code_hash));
        hash_buffer_offset += sizeof(ever_wallet_code_hash);

        // Data hash
        memcpy(hash_buffer + hash_buffer_offset, data_hash, sizeof(data_hash));
        hash_buffer_offset += sizeof(data_hash);

        int result = cx_hash_sha256(hash_buffer, hash_buffer_offset, address, HASH_SIZE);
        VALIDATE(result == HASH_SIZE, ERR_INVALID_HASH);
    }
}

void compute_multisig_address(uint32_t account_number,
                              const uint8_t* wallet,
                              uint16_t wallet_size,
                              uint8_t* address) {
    {
        ByteStream_t src;
        ByteStream_init(&src, (uint8_t*) wallet, wallet_size);
        deserialize_cells_tree(&src);
    }

    BocContext_t* bc = &boc_context;

    VALIDATE(bc->cells_count != 0, ERR_INVALID_CONTRACT);
    find_public_key_cell();  // sets public key cell index to boc_context

    VALIDATE(bc->public_key_cell_index && bc->public_key_label_size_bits, ERR_INVALID_CONTRACT);
    Cell_t* cell = &bc->cells[bc->public_key_cell_index];
    uint8_t cell_data_size = Cell_get_data_size(cell);
    VALIDATE(cell_data_size != 0 && cell_data_size <= MAX_PUBLIC_KEY_CELL_DATA_SIZE,
             ERR_INVALID_CONTRACT);
    uint8_t* cell_data = Cell_get_data(cell);

    memcpy(bc->public_key_cell_data, cell_data, cell_data_size);
    uint8_t* public_key = data_context.pk_context.public_key;
    VALIDATE(get_public_key(account_number, public_key) == 0, ERR_GET_PUBLIC_KEY_FAILED);

    uint8_t* data = bc->public_key_cell_data;
    SliceData_t slice;
    SliceData_init(&slice, data, sizeof(bc->public_key_cell_data));
    SliceData_move_by(&slice, bc->public_key_label_size_bits);
    SliceData_append(&slice, public_key, PUBLIC_KEY_LENGTH * 8, true);

    // Hash all cells in reverse order (children before parents).
    // The pruned code cell is included: calc_cell_hash extracts its hash
    // directly from the cell data, so no manual injection is needed.
    for (int16_t i = bc->cells_count - 1; i >= 0; --i) {
        Cell_t* it_cell = &bc->cells[i];
        calc_cell_hash(it_cell, i);
    }

    memcpy(address, bc->hashes, HASH_SIZE);
}

void compute_wallet_v5r1_address(uint32_t account_number, uint8_t* address) {
    uint8_t data_hash[HASH_SIZE];

    // Compute data hash
    // Cell data: is_signature_allowed(1) + seqno(32) + wallet_id(32) + pubkey(256) + extensions(1)
    // = 322 data bits + completion tag → 41 bytes
    {
        uint8_t hash_buffer[43];  // d1(1) + d2(1) + data(41)
        memset(hash_buffer, 0, sizeof(hash_buffer));

        uint16_t hash_buffer_offset = 0;

        hash_buffer[0] = 0x00;  // d1(1)
        hash_buffer[1] = 0x51;  // d2(1)
        hash_buffer_offset += 2;

        // Bit stream: [1] [seqno(32)] [wallet_id(32)] [pubkey(256)] [0] [tag]
        // All data shifted by 1 bit due to leading is_signature_allowed=1
        uint8_t source[40];  // seqno(4) + wallet_id(4) + pubkey(32)
        memset(source, 0, sizeof(uint32_t));
        writeUint32BE(TON_WALLET_V5R1_ID, source + sizeof(uint32_t));

        // Pubkey
        uint8_t public_key[PUBLIC_KEY_LENGTH];
        VALIDATE(get_public_key(account_number, public_key) == 0, ERR_GET_PUBLIC_KEY_FAILED);

        memcpy(source + 8, public_key, PUBLIC_KEY_LENGTH);

        // Pack with 1-bit offset
        hash_buffer[hash_buffer_offset] = 0x80;  // is_signature_allowed = 1
        for (int i = 0; i < 40; i++) {
            hash_buffer[hash_buffer_offset + i] |= source[i] >> 1;
            hash_buffer[hash_buffer_offset + i + 1] = source[i] << 7;
        }
        // extensions = 0 (bit 321, already zero after shift)
        // Completion tag (bit 322): marks end of non-byte-aligned cell data
        hash_buffer[hash_buffer_offset + 40] |= 0x20;

        // Calculate data hash
        int result = cx_hash_sha256(hash_buffer, sizeof(hash_buffer), data_hash, HASH_SIZE);
        VALIDATE(result == HASH_SIZE, ERR_INVALID_HASH);
    }

    // Compute address = repr_hash(StateInit cell)
    //
    // StateInit TL-B: split_depth=0, special=0, code=1, data=1, library=0
    // Bits: 00110 + completion tag → 00110_100 = 0x34
    // code=1 - code cell; data=1 - data cell
    //
    // TON repr_hash:
    //   SHA256( d1 | d2 | data | depth_ref[0] | depth_ref[1] | hash_ref[0] | hash_ref[1] )
    //
    // d1 = 0x02
    // d2 = 0x01
    // data = 0x34
    // depth_ref[0] = code_depth (2 bytes BE) = 6
    // depth_ref[1] = data_depth (2 bytes BE) = 0
    // hash_ref[0] = code_hash (32 bytes)
    // hash_ref[1] = data_hash (32 bytes)
    {
        uint8_t hash_buffer[71];  // d1(1) + d2(1) + data(1) + depths(4) + code_hash(32) + data_hash(32)

        uint16_t hash_buffer_offset = 0;
        hash_buffer[0] = 0x02;  // d1
        hash_buffer[1] = 0x01;  // d2
        hash_buffer_offset += 2;

        hash_buffer[2] = 0x34;  // StateInit data
        hash_buffer_offset += 1;

        // code_depth(2 bytes BE) + data_depth(2 bytes BE) = 0x0006 0x0000
        writeUint32BE(0x60000, hash_buffer + hash_buffer_offset);
        hash_buffer_offset += sizeof(uint32_t);

        // Code hash
        memcpy(hash_buffer + hash_buffer_offset,
               wallet_v5r1_code_hash,
               sizeof(wallet_v5r1_code_hash));
        hash_buffer_offset += sizeof(wallet_v5r1_code_hash);

        // Data hash
        memcpy(hash_buffer + hash_buffer_offset, data_hash, sizeof(data_hash));
        hash_buffer_offset += sizeof(data_hash);

        int result = cx_hash_sha256(hash_buffer, hash_buffer_offset, address, HASH_SIZE);
        VALIDATE(result == HASH_SIZE, ERR_INVALID_HASH);
    }
}

void compute_wallet_v4_address(uint32_t account_number,
                                const uint8_t* code_hash,
                                uint8_t* address) {
    uint8_t data_hash[HASH_SIZE];

    // Data cell: seqno(32) + wallet_id(32) + pubkey(256) + plugin_dict_empty(1) = 321 bits
    // d1=0x00, d2=0x51, 41 data bytes
    {
        uint8_t hash_buffer[43];  // d1(1) + d2(1) + data(41)
        uint16_t hash_buffer_offset = 0;

        hash_buffer[0] = 0x00;  // d1
        hash_buffer[1] = 0x51;  // d2
        hash_buffer_offset += 2;

        // seqno(32)=0 as high 4 bytes, wallet_id(32)=TON_WALLET_ID as low 4 bytes
        writeUint64BE(TON_WALLET_ID, hash_buffer + hash_buffer_offset);
        hash_buffer_offset += sizeof(uint64_t);

        // Pubkey
        uint8_t public_key[PUBLIC_KEY_LENGTH];
        VALIDATE(get_public_key(account_number, public_key) == 0, ERR_GET_PUBLIC_KEY_FAILED);

        memcpy(hash_buffer + hash_buffer_offset, public_key, PUBLIC_KEY_LENGTH);
        hash_buffer_offset += PUBLIC_KEY_LENGTH;

        // plugin_dict_empty(1)=0 + completion_tag(1)=1 → 0x40
        hash_buffer[hash_buffer_offset] = 0x40;
        hash_buffer_offset += 1;

        int result = cx_hash_sha256(hash_buffer, hash_buffer_offset, data_hash, HASH_SIZE);
        VALIDATE(result == HASH_SIZE, ERR_INVALID_HASH);
    }

    // StateInit repr_hash: SHA256(d1 | d2 | 0x34 | code_depth(2B) | data_depth(2B) | code_hash | data_hash)
    // code_depth = 7, data_depth = 0
    {
        uint8_t hash_buffer[71];  // d1(1) + d2(1) + data(1) + depths(4) + code_hash(32) + data_hash(32)
        uint16_t hash_buffer_offset = 0;

        hash_buffer[0] = 0x02;  // d1
        hash_buffer[1] = 0x01;  // d2
        hash_buffer_offset += 2;

        hash_buffer[2] = 0x34;  // StateInit data
        hash_buffer_offset += 1;

        // code_depth=7, data_depth=0
        writeUint32BE(0x70000, hash_buffer + hash_buffer_offset);
        hash_buffer_offset += sizeof(uint32_t);

        // Code hash
        memcpy(hash_buffer + hash_buffer_offset, code_hash, HASH_SIZE);
        hash_buffer_offset += HASH_SIZE;

        // Data hash
        memcpy(hash_buffer + hash_buffer_offset, data_hash, HASH_SIZE);
        hash_buffer_offset += HASH_SIZE;

        int result = cx_hash_sha256(hash_buffer, hash_buffer_offset, address, HASH_SIZE);
        VALIDATE(result == HASH_SIZE, ERR_INVALID_HASH);
    }
}

void get_address(const uint32_t account_number, uint8_t wallet_type, uint8_t* address) {
    switch (wallet_type) {
        case WALLET_V3: {
            compute_wallet_v3_address(account_number, WALLET_ID, wallet_v3_code_hash, address);
            break;
        }
        case EVER_WALLET: {
            compute_ever_wallet_address(account_number, address);
            break;
        }
        case SAFE_MULTISIG_WALLET: {
            compute_multisig_address(account_number,
                                     safe_multisig_wallet,
                                     sizeof(safe_multisig_wallet),
                                     address);
            break;
        }
        case SAFE_MULTISIG_WALLET_24H: {
            compute_multisig_address(account_number,
                                     safe_multisig_wallet_24h,
                                     sizeof(safe_multisig_wallet_24h),
                                     address);
            break;
        }
        case SETCODE_MULTISIG_WALLET: {
            compute_multisig_address(account_number,
                                     setcode_multisig_wallet,
                                     sizeof(setcode_multisig_wallet),
                                     address);
            break;
        }
        case BRIDGE_MULTISIG_WALLET: {
            compute_multisig_address(account_number,
                                     bridge_multisig_wallet,
                                     sizeof(bridge_multisig_wallet),
                                     address);
            break;
        }
        case SURF_WALLET: {
            compute_multisig_address(account_number,
                                     surf_wallet,
                                     sizeof(surf_wallet),
                                     address);
            break;
        }
        case MULTISIG_2: {
            compute_multisig_address(account_number,
                                     multisig_2_wallet,
                                     sizeof(multisig_2_wallet),
                                     address);
            break;
        }
        case MULTISIG_2_1: {
            compute_multisig_address(account_number,
                                     multisig_2_1_wallet,
                                     sizeof(multisig_2_1_wallet),
                                     address);
            break;
        }
        case WALLET_V5R1: {
            compute_wallet_v5r1_address(account_number, address);
            break;
        }
        case WALLET_V4R1: {
            compute_wallet_v4_address(account_number, wallet_v4r1_code_hash, address);
            break;
        }
        case WALLET_V4R2: {
            compute_wallet_v4_address(account_number, wallet_v4r2_code_hash, address);
            break;
        }
        case WALLET_V3R1: {
            compute_wallet_v3_address(account_number, TON_WALLET_ID, wallet_v3r1_code_hash, address);
            break;
        }
        case WALLET_V3R2: {
            compute_wallet_v3_address(account_number, TON_WALLET_ID, wallet_v3_code_hash, address);
            break;
        }
        default:
            THROW(ERR_INVALID_WALLET_TYPE);
    }
}

#else

void get_address(const uint32_t account_number, uint8_t wallet_type, uint8_t* address) {
}

#endif
