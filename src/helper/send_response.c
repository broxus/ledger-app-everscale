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

int helper_send_response_address() {
    uint8_t resp[1 + ADDRESS_LENGTH] = {0};
    size_t offset = 0;
    resp[offset++] = ADDRESS_LENGTH;
    memmove(resp + offset, data_context.addr_context.address, ADDRESS_LENGTH);
    offset += ADDRESS_LENGTH;
    reset_app_context();
    return io_send_response_pointer(resp, offset, SUCCESS);
}

int helper_send_response_sign() {
    uint8_t resp[1 + SIGNATURE_LENGTH] = {0};
    size_t offset = 0;
    resp[offset++] = SIGNATURE_LENGTH;
    memmove(resp + offset, data_context.sign_context.signature, SIGNATURE_LENGTH);
    offset += SIGNATURE_LENGTH;
    return io_send_response_pointer(resp, offset, SUCCESS);
}

int helper_send_response_sign_transaction() {
    uint8_t resp[1 + SIGNATURE_LENGTH] = {0};
    size_t offset = 0;
    resp[offset++] = SIGNATURE_LENGTH;
    memmove(resp + offset, data_context.sign_tr_context.signature, SIGNATURE_LENGTH);
    offset += SIGNATURE_LENGTH;
    return io_send_response_pointer(resp, offset, SUCCESS);
}
