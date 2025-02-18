#include "helper/send_response.h"

int helper_send_response_public_key() {
    uint8_t resp[1 + PUBLIC_KEY_LENGTH] = {0};
    size_t offset = 0;
    resp[offset++] = PUBLIC_KEY_LENGTH;
    memmove(resp + offset, data_context.pk_context.public_key, PUBLIC_KEY_LENGTH);
    offset += PUBLIC_KEY_LENGTH;
    reset_app_context();
    return io_send_response_pointer(resp, offset, SUCCESS);
}

uint8_t helper_send_response_address() {
    uint8_t tx = 0;
    G_io_apdu_buffer[tx++] = ADDRESS_LENGTH;
    memmove(G_io_apdu_buffer + tx, data_context.addr_context.address, ADDRESS_LENGTH);
    tx += ADDRESS_LENGTH;
    reset_app_context();
    return tx;
}

uint8_t helper_send_response_sign_transaction() {
    cx_ecfp_private_key_t privateKey;
    SignTransactionContext_t* context = &data_context.sign_tr_context;
    cx_err_t error;

    BEGIN_TRY {
        TRY {
            get_private_key(context->account_number, &privateKey);
            if (!context->sign_with_chain_id) {
                error = cx_eddsa_sign_no_throw(&privateKey,
                                               CX_SHA512,
                                               context->to_sign,
                                               TO_SIGN_LENGTH,
                                               context->signature,
                                               SIGNATURE_LENGTH);
            } else {
                error = cx_eddsa_sign_no_throw(&privateKey,
                                               CX_SHA512,
                                               context->to_sign,
                                               CHAIN_ID_LENGTH + TO_SIGN_LENGTH,
                                               context->signature,
                                               SIGNATURE_LENGTH);
            }
            if (error != CX_OK) {
                THROW(ERR_SIGNING_FAILED);
            }
        }
        FINALLY {
            explicit_bzero(&privateKey, sizeof(privateKey));
        }
    }
    END_TRY;

    uint8_t tx = 0;
    G_io_apdu_buffer[tx++] = SIGNATURE_LENGTH;
    memmove(G_io_apdu_buffer + tx, context->signature, SIGNATURE_LENGTH);
    tx += SIGNATURE_LENGTH;
    return tx;
}

uint8_t helper_send_response_sign() {
    cx_ecfp_private_key_t privateKey;
    SignContext_t* context = &data_context.sign_context;
    cx_err_t error;
    BEGIN_TRY {
        TRY {
            get_private_key(context->account_number, &privateKey);
            error = cx_eddsa_sign_no_throw(&privateKey,
                                           CX_SHA512,
                                           context->to_sign,
                                           SIGN_MAGIC_LENGTH + TO_SIGN_LENGTH,
                                           context->signature,
                                           SIGNATURE_LENGTH);
            if (error != CX_OK) {
                THROW(ERR_SIGNING_FAILED);
            }
        }
        FINALLY {
            explicit_bzero(&privateKey, sizeof(privateKey));
        }
    }
    END_TRY;

    uint8_t tx = 0;
    G_io_apdu_buffer[tx++] = SIGNATURE_LENGTH;
    memmove(G_io_apdu_buffer + tx, context->signature, SIGNATURE_LENGTH);
    tx += SIGNATURE_LENGTH;
    return tx;
}
