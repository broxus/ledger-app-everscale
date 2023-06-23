#include "apdu_constants.h"
#include "utils.h"
#include "errors.h"

static uint8_t set_result_sign() {
    cx_ecfp_private_key_t privateKey;
    SignContext_t* context = &data_context.sign_context;

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
    ux_sign_flow_1_step,
    pnn,
    {
      &C_icon_certificate,
      "Sign",
      "message",
    });
UX_STEP_NOCB(
    ux_sign_flow_2_step,
    bnnn_paging,
    {
      .title = "Message hash",
      .text = data_context.sign_context.to_sign_str,
    });
UX_STEP_CB(
    ux_sign_flow_3_step,
    pbb,
    send_response(0, false),
    {
      &C_icon_crossmark,
      "Cancel",
      "signature",
    });
UX_STEP_CB(
    ux_sign_flow_4_step,
    pbb,
    send_response(set_result_sign(), true),
    {
      &C_icon_validate_14,
      "Sign",
      "message",
    });

UX_FLOW(ux_sign_flow,
    &ux_sign_flow_1_step,
    &ux_sign_flow_2_step,
    &ux_sign_flow_3_step,
    &ux_sign_flow_4_step
);

void handleSign(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, __attribute__((unused)) uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
    UNUSED(p2);
    UNUSED(tx);

    if (p1 == P1_NON_CONFIRM) {
        // Don't allow blind signing.
        THROW(0x6808);
    }

    SignContext_t* context = &data_context.sign_context;

    size_t offset = 0;

    context->account_number = readUint32BE(dataBuffer + offset);
    offset += sizeof(context->account_number);

    uint8_t metadata = dataBuffer[offset];
    offset += sizeof(metadata);

    // Read chain id if present
    if (metadata & FLAG_WITH_CHAIN_ID) {
        context->sign_with_chain_id = true;

        memcpy(context->chain_id, dataBuffer + offset, CHAIN_ID_LENGTH);
        offset += sizeof(context->chain_id);
    }

    if (!context->sign_with_chain_id) {
        memcpy(context->to_sign, dataBuffer + offset, TO_SIGN_LENGTH);
        format_hex(context->to_sign, TO_SIGN_LENGTH, context->to_sign_str, sizeof(context->to_sign_str));
    } else {
        memcpy(context->to_sign, context->chain_id, CHAIN_ID_LENGTH);
        memcpy(context->to_sign + CHAIN_ID_LENGTH, dataBuffer + offset, TO_SIGN_LENGTH);
        format_hex(context->to_sign, CHAIN_ID_LENGTH + TO_SIGN_LENGTH, context->to_sign_str, sizeof(context->to_sign_str));
    }

    ux_flow_init(0, ux_sign_flow, NULL);
    *flags |= IO_ASYNCH_REPLY;
}
