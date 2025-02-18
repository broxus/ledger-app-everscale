#include "format.h"
#include "apdu_constants.h"
#include "utils.h"
#include "errors.h"
#include "ui/display.h"

static const char SIGN_MAGIC[] = {0xFF, 0xFF, 0xFF, 0xFF};

void handleSign(uint8_t* dataBuffer,
                __attribute__((unused)) uint16_t dataLength,
                volatile unsigned int* flags) {
    SignContext_t* context = &data_context.sign_context;

    size_t offset = 0;

    VALIDATE(dataLength >= offset + sizeof(context->account_number), ERR_INVALID_REQUEST);
    context->account_number = readUint32BE(dataBuffer + offset);
    offset += sizeof(context->account_number);

    VALIDATE(dataLength >= offset + TO_SIGN_LENGTH, ERR_INVALID_REQUEST);
    memcpy(context->to_sign, SIGN_MAGIC, SIGN_MAGIC_LENGTH);
    memcpy(context->to_sign + SIGN_MAGIC_LENGTH, dataBuffer + offset, TO_SIGN_LENGTH);
    format_hex(context->to_sign,
               SIGN_MAGIC_LENGTH + TO_SIGN_LENGTH,
               context->to_sign_str,
               sizeof(context->to_sign_str));

    ui_display_sign();
    *flags |= IO_ASYNCH_REPLY;
}
