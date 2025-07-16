#include "apdu_constants.h"
#include "buffer.h"
#include "read.h"
#include "utils.h"
#include "io.h"
#include "errors.h"
#include "byte_stream.h"
#include "message.h"
#include "contract.h"
#include "ui/display.h"

int handleSignTransaction(buffer_t* cdata,
                          bool is_first_chunk,
                          bool more,
                          volatile unsigned int* flags) {
    if (is_first_chunk) {
        reset_app_context();
    }

    SignTransactionContext_t* context = &data_context.sign_tr_context;

    if (is_first_chunk) {
        VALIDATE(cdata->size >= cdata->offset + sizeof(context->account_number),
                 ERR_INVALID_REQUEST);
        context->account_number = read_u32_be(cdata->ptr, cdata->offset);
        cdata->offset += sizeof(context->account_number);

        VALIDATE(cdata->size >= cdata->offset + sizeof(context->origin_wallet_type),
                 ERR_INVALID_REQUEST);
        context->origin_wallet_type = cdata->ptr[cdata->offset];
        cdata->offset += sizeof(context->origin_wallet_type);

        VALIDATE(cdata->size >= cdata->offset + sizeof(context->decimals), ERR_INVALID_REQUEST);
        context->decimals = cdata->ptr[cdata->offset];
        cdata->offset += sizeof(context->decimals);

        VALIDATE(cdata->size >= cdata->offset + sizeof(uint8_t), ERR_INVALID_REQUEST);
        uint8_t ticker_len = cdata->ptr[cdata->offset];
        cdata->offset += sizeof(ticker_len);

        VALIDATE(ticker_len != 0 && ticker_len <= MAX_TICKER_LEN, ERR_TICKER_LENGTH);

        VALIDATE(cdata->size >= cdata->offset + ticker_len, ERR_INVALID_REQUEST);
        memcpy(context->ticker, cdata->ptr + cdata->offset, ticker_len);
        cdata->offset += ticker_len;

        VALIDATE(cdata->size >= cdata->offset + sizeof(uint8_t), ERR_INVALID_REQUEST);
        uint8_t metadata = cdata->ptr[cdata->offset];
        cdata->offset += sizeof(metadata);

        // Read wallet type if present
        if (metadata & FLAG_WITH_WALLET_ID) {
            VALIDATE(cdata->size >= cdata->offset + sizeof(context->current_wallet_type),
                     ERR_INVALID_REQUEST);
            context->current_wallet_type = cdata->ptr[cdata->offset];
            cdata->offset += sizeof(context->current_wallet_type);
        } else {
            context->current_wallet_type = context->origin_wallet_type;
        }

        // Get address
        get_address(context->account_number, context->origin_wallet_type, context->address);
        memset(&boc_context, 0, sizeof(boc_context));

        // Read wc if present
        context->wc = DEFAULT_WORKCHAIN_ID;
        if (metadata & FLAG_WITH_WORKCHAIN_ID) {
            VALIDATE(cdata->size >= cdata->offset + sizeof(context->wc), ERR_INVALID_REQUEST);
            context->wc = cdata->ptr[cdata->offset];
            cdata->offset += sizeof(context->wc);
        }

        // Read initial address if present
        if (metadata & FLAG_WITH_ADDRESS) {
            VALIDATE(cdata->size >= cdata->offset + sizeof(context->address), ERR_INVALID_REQUEST);
            memcpy(context->prepend_address, cdata->ptr + cdata->offset, ADDRESS_LENGTH);
            cdata->offset += sizeof(context->address);
        } else {
            memcpy(context->prepend_address, context->address, ADDRESS_LENGTH);
        }

        // Read chain id if present
        if (metadata & FLAG_WITH_CHAIN_ID) {
            context->sign_with_chain_id = true;

            VALIDATE(cdata->size >= cdata->offset + sizeof(context->chain_id), ERR_INVALID_REQUEST);
            memcpy(context->chain_id, cdata->ptr + cdata->offset, CHAIN_ID_LENGTH);
            cdata->offset += sizeof(context->chain_id);
        } else {
            context->sign_with_chain_id = false;
        }
    }
    // cdata->offset is a pointer to cdata->ptr, or the number of bytes we moved. here +
    // cdata->offset means start of the message we need to save data to a context buffer and add
    // msg_length to cdata->offset of this buffer
    uint8_t* msg_begin = (uint8_t*) (cdata->ptr + cdata->offset);

    // Since we check LC cdata->size can not be manipulated
    uint16_t msg_length = cdata->size - cdata->offset;

    if (msg_begin && msg_length > 0) {  // if data exists
        VALIDATE(context->data_offset + msg_length < sizeof(context->data), ERR_INVALID_DATA);
        memcpy(context->data + context->data_offset, msg_begin, msg_length);
        context->data_offset += msg_length;  // add length of the new message to our context offset
    }

    if (more) {
        return io_send_sw(SUCCESS);
    }

    // Handle transaction
    ByteStream_t src;
    ByteStream_init(&src, context->data, context->data_offset);

    int flow = prepare_to_sign(&src, context->wc, context->address, context->prepend_address);

    ui_display_sign_transaction(flow);

    *flags |= IO_ASYNCH_REPLY;
    return 0;
}
