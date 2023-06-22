#include "apdu_constants.h"
#include "utils.h"
#include "errors.h"
#include "byte_stream.h"
#include "message.h"
#include "contract.h"

static uint8_t set_result_sign_transaction() {
    cx_ecfp_private_key_t privateKey;
    SignTransactionContext_t* context = &data_context.sign_tr_context;

    BEGIN_TRY {
        TRY {
            get_private_key(context->account_number, &privateKey);
            if (!context->sign_with_chain_id) {
                cx_eddsa_sign(&privateKey, CX_LAST, CX_SHA512, context->to_sign, TO_SIGN_LENGTH, NULL, 0, context->signature, SIGNATURE_LENGTH, NULL);
            } else {
                cx_eddsa_sign(&privateKey, CX_LAST, CX_SHA512, context->to_sign, CHAIN_ID_LENGTH + TO_SIGN_LENGTH, NULL, 0, context->signature, SIGNATURE_LENGTH, NULL);
            }
        } FINALLY {
            memset(&privateKey, 0, sizeof(privateKey));
        }
    }
    END_TRY;

    uint8_t tx = 0;
    G_io_apdu_buffer[tx++] = SIGNATURE_LENGTH;
    memmove(G_io_apdu_buffer + tx, context->signature, SIGNATURE_LENGTH);
    tx += SIGNATURE_LENGTH;
    return tx;
}

UX_STEP_NOCB(
    ux_sign_transaction_intro,
    pnn,
    {
      &C_icon_eye,
      "Review",
      "transaction",
    });
UX_STEP_NOCB(
    ux_sign_transaction_burn,
    bnnn_paging,
    {
      .title = "Action",
      .text = "Burn"
    });
UX_STEP_NOCB(
    ux_sign_transaction_deploy,
    bnnn_paging,
    {
      .title = "Action",
      .text = "Deploy"
    });
UX_STEP_NOCB(
    ux_sign_transaction_confirm,
    bnnn_paging,
    {
      .title = "Action",
      .text = "Confirm"
    });
UX_STEP_NOCB(
    ux_sign_transaction_transfer,
    bnnn_paging,
    {
      .title = "Action",
      .text = "Transfer"
    });
UX_STEP_NOCB(
    ux_sign_transaction_amount,
    bnnn_paging,
    {
      .title = "Amount",
      .text = data_context.sign_tr_context.amount_str,
    });
UX_STEP_NOCB(
    ux_sign_transaction_address,
    bnnn_paging,
    {
      .title = "Address",
      .text = data_context.sign_tr_context.address_str,
    });
UX_STEP_NOCB(
    ux_sign_transaction_transaction_id,
    bnnn_paging,
    {
      .title = "Transaction id",
      .text = data_context.sign_tr_context.transaction_id_str,
    });
UX_STEP_CB(
    ux_sign_transaction_accept,
    pbb,
    send_response(set_result_sign_transaction(), true),
    {
      &C_icon_validate_14,
      "Accept",
      "and send",
    });
UX_STEP_CB(
    ux_sign_transaction_reject,
    pb,
    send_response(0, false),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_FLOW(ux_sign_transaction_burn_flow,
    &ux_sign_transaction_intro,
    &ux_sign_transaction_burn,
    &ux_sign_transaction_amount,
    &ux_sign_transaction_accept,
    &ux_sign_transaction_reject
);

UX_FLOW(ux_sign_transaction_deploy_flow,
    &ux_sign_transaction_intro,
    &ux_sign_transaction_deploy,
    &ux_sign_transaction_address,
    &ux_sign_transaction_accept,
    &ux_sign_transaction_reject
);

UX_FLOW(ux_sign_transaction_confirm_flow,
    &ux_sign_transaction_intro,
    &ux_sign_transaction_confirm,
    &ux_sign_transaction_transaction_id,
    &ux_sign_transaction_accept,
    &ux_sign_transaction_reject
);

UX_FLOW(ux_sign_transaction_transfer_flow,
    &ux_sign_transaction_intro,
    &ux_sign_transaction_transfer,
    &ux_sign_transaction_amount,
    &ux_sign_transaction_address,
    &ux_sign_transaction_accept,
    &ux_sign_transaction_reject
);

void handleSignTransaction(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(p2);
    UNUSED(tx);

    if (p1 == P1_NON_CONFIRM) {
        // Don't allow blind signing.
        THROW(0x6808);
    }

    SignTransactionContext_t* context = &data_context.sign_tr_context;

    size_t offset = 0;

    context->account_number = readUint32BE(dataBuffer + offset);
    offset += sizeof(context->account_number);

    context->origin_wallet_type = dataBuffer[offset];
    offset += sizeof(context->origin_wallet_type);

    context->decimals = dataBuffer[offset];
    offset += sizeof(context->decimals);

    uint8_t ticker_len = dataBuffer[offset];
    offset += sizeof(ticker_len);

    VALIDATE(ticker_len != 0 && ticker_len <= MAX_TICKER_LEN, ERR_TICKER_LENGTH);

    memcpy(context->ticker, dataBuffer + offset, ticker_len);
    offset += ticker_len;

    uint8_t metadata = dataBuffer[offset];
    offset += sizeof(metadata);

    // Read wallet type if present
    if (metadata & FLAG_WITH_WALLET_ID) {
        context->current_wallet_type = dataBuffer[offset];
        offset += sizeof(context->current_wallet_type);
    } else {
        context->current_wallet_type = context->origin_wallet_type;
    }

    // Get address
    uint8_t address[ADDRESS_LENGTH];
    get_address(context->account_number, context->origin_wallet_type, address);
    memset(&boc_context, 0, sizeof(boc_context));

    // Read wc if present
    uint8_t wc = DEFAULT_WORKCHAIN_ID;
    if (metadata & FLAG_WITH_WORKCHAIN_ID) {
        wc = dataBuffer[offset];
        offset += sizeof(wc);
    }

    // Read initial address if present
    uint8_t prepend_address[ADDRESS_LENGTH];
    if (metadata & FLAG_WITH_ADDRESS) {
        memcpy(prepend_address, dataBuffer + offset, ADDRESS_LENGTH);
        offset += sizeof(address);
    } else {
        memcpy(prepend_address, address, ADDRESS_LENGTH);
    }

    // Read chain id if present
    if (metadata & FLAG_WITH_CHAIN_ID) {
        context->sign_with_chain_id = true;

        memcpy(context->chain_id, dataBuffer + offset, CHAIN_ID_LENGTH);
        offset += sizeof(context->chain_id);
    } else {
        context->sign_with_chain_id = false;
    }

    uint8_t* msg_begin = dataBuffer + offset;

    // Since we check LC dataLength can not be manipulated
    uint8_t msg_length = dataLength - offset;

    ByteStream_t src;
    ByteStream_init(&src, msg_begin, msg_length);

    int flow = prepare_to_sign(&src, wc, address, prepend_address);

    switch (flow) {
        case SIGN_TRANSACTION_FLOW_TRANSFER:
            ux_flow_init(0, ux_sign_transaction_transfer_flow, NULL);
            break;
        case SIGN_TRANSACTION_FLOW_DEPLOY:
            ux_flow_init(0, ux_sign_transaction_deploy_flow, NULL);
            break;
        case SIGN_TRANSACTION_FLOW_CONFIRM:
            ux_flow_init(0, ux_sign_transaction_confirm_flow, NULL);
            break;
        case SIGN_TRANSACTION_FLOW_BURN:
            ux_flow_init(0, ux_sign_transaction_burn_flow, NULL);
            break;
        default:
            THROW(ERR_INVALID_REQUEST);
    }

    *flags |= IO_ASYNCH_REPLY;
}
