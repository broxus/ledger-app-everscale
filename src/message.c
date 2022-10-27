#include "message.h"
#include "utils.h"
#include "errors.h"
#include "slice_data.h"
#include "byte_stream.h"
#include "contract.h"

#define ROOT_CELL_INDEX 0
#define GIFT_CELL_INDEX 1

void deserialize_array(uint8_t* in, uint8_t in_size, uint16_t offset, uint8_t* out, uint8_t out_size) {
    uint8_t shift = offset % 8;
    uint8_t first_data_byte = offset / 8;
    for (uint16_t i = first_data_byte, j = 0; j < out_size; ++i, ++j) {
        VALIDATE(i == (j + first_data_byte) && (i + 1) <= in_size, ERR_INVALID_DATA);

        uint8_t cur = in[i] << shift;
        out[j] = cur;

        if (j == out_size - 1) {
            out[j] |= in[i + 1] >> (8 - shift);
        }

        if (i != first_data_byte) {
            out[j - 1] |= in[i] >> (8 - shift);
        }
    }
}

void deserialize_address(struct SliceData_t* slice, int8_t* wc, uint8_t* address) {
    uint8_t addr_type = SliceData_get_next_int(slice, 2);
    VALIDATE(addr_type == 0 || addr_type == 2, ERR_INVALID_DATA);

    if (addr_type == 2) {
        uint8_t anycast = SliceData_get_next_bit(slice);
        UNUSED(anycast);

        *wc = (int8_t)SliceData_get_next_byte(slice);

        uint8_t* data = SliceData_begin(slice);
        uint16_t offset = SliceData_get_cursor(slice);
        uint16_t data_size = SliceData_data_size(slice);

        deserialize_array(data, data_size, offset, address, ADDRESS_LENGTH);

        SliceData_move_by(slice, ADDRESS_LENGTH * 8);
    }
}

void deserialize_amount(struct SliceData_t* slice, uint8_t* amount, uint8_t amount_length) {
    uint8_t* data = SliceData_begin(slice);
    uint16_t data_size = SliceData_data_size(slice);

    uint16_t offset = SliceData_get_cursor(slice);
    deserialize_array(data, data_size, offset, amount, amount_length);
    SliceData_move_by(slice, amount_length * 8);
}

void set_dst_address(uint8_t wc, const uint8_t* address) {
    char wc_temp[6]; // snprintf always returns zero
    snprintf(wc_temp, sizeof(wc_temp), "%d:", wc);
    int wc_len = strlen(wc_temp);

    char* address_str = data_context.sign_tr_context.address_str;
    memcpy(address_str, wc_temp, wc_len);
    address_str += wc_len;

    snprintf(address_str, sizeof(data_context.sign_tr_context.address_str) - wc_len, "%.*H", ADDRESS_LENGTH, address);
}

void set_amount(const uint8_t* amount, uint8_t amount_length, uint8_t flags) {
    switch (flags) {
        case NORMAL_FLAG: {
            char* amount_str = data_context.sign_tr_context.amount_str;
            uint8_t text_size = convert_hex_amount_to_displayable(amount, EVER_DECIMALS, amount_length, amount_str);
            strcpy(amount_str + text_size, " EVER");
            break;
        }
        case ALL_BALANCE_FLAG: {
            strcpy(data_context.sign_tr_context.amount_str, "All balance");
            break;
        }
        case ALL_BALANCE_AND_DELETE_FLAG: {
            strcpy(data_context.sign_tr_context.amount_str, "All balance and delete account");
            break;
        }
        default:
            THROW(ERR_INVALID_DATA);
    }
}

void set_all_balance() {
    char* amount_str = data_context.sign_tr_context.amount_str;
    strcpy(amount_str, "All balance");
}

void set_all_balance_and_delete() {
    char* amount_str = data_context.sign_tr_context.amount_str;
    strcpy(amount_str, "All balance and delete account");
}

void deserialize_message_header(struct SliceData_t* slice, uint8_t flags) {
    uint8_t int_msg = SliceData_get_next_bit(slice);
    VALIDATE(!int_msg, ERR_INVALID_DATA);

    uint8_t ihr_disabled = SliceData_get_next_bit(slice);
    UNUSED(ihr_disabled);

    uint8_t bounce = SliceData_get_next_bit(slice);
    UNUSED(bounce);

    uint8_t bounced = SliceData_get_next_bit(slice);
    UNUSED(bounced);

    // Sender address
    int8_t src_wc;
    uint8_t src_addr[ADDRESS_LENGTH];
    deserialize_address(slice, &src_wc, src_addr);

    // Recipient address
    int8_t dst_wc;
    uint8_t dst_address[ADDRESS_LENGTH];
    deserialize_address(slice, &dst_wc, dst_address);

    set_dst_address(dst_wc, dst_address);

    // Amount
    uint8_t amount_length = SliceData_get_next_int(slice, 4);
    uint8_t amount[amount_length];
    deserialize_amount(slice, amount, amount_length);
    set_amount(amount, amount_length, flags);
}

void deserialize_message(struct SliceData_t* slice, uint8_t flags) {
    deserialize_message_header(slice, flags);
}

void deserialize_multisig_params(struct SliceData_t* slice, uint32_t function_id, uint8_t* address) {
    switch (function_id) {
        case MULTISIG_DEPLOY_TRANSACTION: {
            DataContext_t * dc = &data_context;

            // Address to deploy
            set_dst_address(DEFAULT_WORKCHAIN, address);

            // Attached amount
            uint8_t amount[sizeof(uint32_t)];
            writeUint32BE(DEFAULT_ATTACHED_AMOUNT, amount);

            set_amount(amount, sizeof(amount), NORMAL_FLAG);

            break;
        }
        case MULTISIG_SEND_TRANSACTION: {
            // Recipient address
            int8_t dst_wc;
            uint8_t dst_address[ADDRESS_LENGTH];
            deserialize_address(slice, &dst_wc, dst_address);

            set_dst_address(dst_wc, dst_address);

            // Amount
            uint8_t amount_length = 16;
            uint8_t amount[amount_length];
            deserialize_amount(slice, amount, amount_length);

            uint8_t bounce = SliceData_get_next_bit(slice);
            UNUSED(bounce);

            uint8_t flags = SliceData_get_next_byte(slice);

            set_amount(amount, amount_length, flags);

            break;
        }
        case MULTISIG_SUBMIT_TRANSACTION: {
            // Recipient address
            int8_t dst_wc;
            uint8_t dst_address[ADDRESS_LENGTH];
            deserialize_address(slice, &dst_wc, dst_address);

            set_dst_address(dst_wc, dst_address);

            // Amount
            uint8_t amount_length = 16;
            uint8_t amount[amount_length];
            deserialize_amount(slice, amount, amount_length);

            uint8_t bounce = SliceData_get_next_bit(slice);
            UNUSED(bounce);

            uint8_t all_balance = SliceData_get_next_bit(slice);

            uint8_t flags;
            if (!all_balance) {
                flags = NORMAL_FLAG;
            } else {
                flags = ALL_BALANCE_FLAG;
            }

            set_amount(amount, amount_length, flags);

            break;
        }
        default:
            THROW(ERR_INVALID_FUNCTION_ID);
    }
}

void prepare_payload_hash(BocContext_t* bc) {
    for (int16_t i = bc->cells_count - 1; i >= 0; --i) {
        Cell_t* cell = &bc->cells[i];
        calc_cell_hash(cell, i);
    }

    memcpy(data_context.sign_tr_context.to_sign, &bc->hashes[ROOT_CELL_INDEX * HASH_SIZE], TO_SIGN_LENGTH);
}

uint32_t deserialize_wallet_v3(struct SliceData_t* slice) {
    uint32_t id = SliceData_get_next_int(slice, 32);
    VALIDATE(id == WALLET_ID, ERR_INVALID_DATA);

    uint32_t expire_at = SliceData_get_next_int(slice, 32);
    UNUSED(expire_at);

    uint32_t seqno = SliceData_get_next_int(slice, 32);
    UNUSED(seqno);

    uint8_t flags = SliceData_get_next_byte(slice);

    uint16_t remaining_bits = SliceData_remaining_bits(slice);
    VALIDATE(remaining_bits == 0, ERR_INVALID_DATA);

    return flags;
}

uint32_t deserialize_contract_header(struct SliceData_t* slice) {
    uint8_t is_pubkey_present = SliceData_get_next_bit(slice);
    if (is_pubkey_present) {
        SliceData_move_by(slice, PUBLIC_KEY_LENGTH * 8);
    }

    uint64_t time = SliceData_get_next_int(slice, 64);
    UNUSED(time);

    uint64_t expire = SliceData_get_next_int(slice, 32);
    UNUSED(expire);

    uint32_t function_id = SliceData_get_next_int(slice, 32);
    return function_id;
}

void prepend_address_to_cell(uint8_t* cell_buffer, uint16_t cell_buffer_size, struct Cell_t* cell, uint8_t* address) {
    uint16_t bit_len = Cell_bit_len(cell);
    bit_len += 267; // Prefix(2) + Anycast(1) + WorkchainId(8) + Address(32 * 8)

    uint8_t d1 = Cell_get_d1(cell);
    uint8_t d2 = ((bit_len >> 2) & 0b11111110) | (bit_len % 8 != 0);

    cell_buffer[0] = d1;
    cell_buffer[1] = d2;

    SliceData_t slice;
    SliceData_init(&slice, cell_buffer + CELL_DATA_OFFSET, cell_buffer_size);

    // Append prefix
    uint8_t prefix[] = { 0x80 }; // $100 prefix AddrStd
    SliceData_append(&slice, prefix, 3, false);

    // Append workchain
    uint8_t wc[] = { 0x00 };
    SliceData_append(&slice, wc, 8, false);

    // Append address
    SliceData_append(&slice, address, ADDRESS_LENGTH * 8, false);

    // Append cell data
    uint8_t* data = Cell_get_data(cell);
    uint8_t data_size = Cell_get_data_size(cell);
    SliceData_append(&slice, data, data_size * 8, false);

    // Append references
    uint8_t refs_count = 0;
    uint8_t* refs = Cell_get_refs(cell, &refs_count);

    VALIDATE(refs_count >= 0 && refs_count <= MAX_REFERENCES_COUNT, ERR_INVALID_DATA);
    for (uint8_t child = 0; child < refs_count; ++child) {
        uint8_t cell_data_size = (d2 >> 1) + (((d2 & 1) != 0) ? 1 : 0);
        cell_buffer[CELL_DATA_OFFSET + cell_data_size + child] = refs[child];
    }

    // Replace cell
    cell->cell_begin = cell_buffer;
}

void prepare_to_sign(struct ByteStream_t* src) {
    // Init context
    BocContext_t* bc = &boc_context;
    DataContext_t * dc = &data_context;

    // Get address
    uint8_t address[ADDRESS_LENGTH];
    get_address(dc->sign_tr_context.account_number, dc->sign_tr_context.wallet_type, address);
    memset(bc, 0, sizeof(boc_context));

    // Parse transaction boc
    deserialize_cells_tree(src);

    Cell_t* root_cell = &bc->cells[ROOT_CELL_INDEX];

    SliceData_t root_slice;
    SliceData_from_cell(&root_slice, root_cell);

    switch (dc->sign_tr_context.wallet_type) {
        case WALLET_V3: {
            uint8_t flags = deserialize_wallet_v3(&root_slice);

            Cell_t* gift_cell = &bc->cells[GIFT_CELL_INDEX];

            SliceData_t gift_slice;
            SliceData_from_cell(&gift_slice, gift_cell);

            deserialize_message(&gift_slice, flags);

            // Calculate payload hash to sign
            prepare_payload_hash(bc);

            break;
        }
        case SAFE_MULTISIG_WALLET:
        case SAFE_MULTISIG_WALLET_24H:
        case SETCODE_MULTISIG_WALLET:
        case BRIDGE_MULTISIG_WALLET:
        case SURF_WALLET: {
            uint32_t function_id = deserialize_contract_header(&root_slice);

            Cell_t* gift_cell = &bc->cells[GIFT_CELL_INDEX];

            SliceData_t gift_slice;
            SliceData_from_cell(&gift_slice, gift_cell);

            deserialize_multisig_params(&gift_slice, function_id, address);

            // Calculate payload hash to sign
            prepare_payload_hash(bc);

            break;
        }
        case EVER_WALLET:
        case MULTISIG_2: {
            uint32_t function_id = deserialize_contract_header(&root_slice);

            Cell_t* gift_cell = &bc->cells[GIFT_CELL_INDEX];

            SliceData_t gift_slice;
            SliceData_from_cell(&gift_slice, gift_cell);

            deserialize_multisig_params(&gift_slice, function_id, address);

            // Prepend address to root cell
            uint8_t cell_buffer[130]; // d1(1) + d2(1) + data(128)
            prepend_address_to_cell(cell_buffer, sizeof(cell_buffer), root_cell, address);

            // Calculate payload hash to sign
            prepare_payload_hash(bc);

            break;
        }
        default:
            THROW(ERR_INVALID_WALLET_TYPE);
    }
}
