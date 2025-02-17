#include "os.h"
#include "ux.h"
#include "utils.h"
#include "apdu_constants.h"
#include "errors.h"
#include "ui/display.h"
#include "response_setter.h"

void handleGetPublicKey(uint8_t p1,
                        uint8_t p2,
                        uint8_t* dataBuffer,
                        uint16_t dataLength,
                        volatile unsigned int* flags,
                        volatile unsigned int* tx) {
    VALIDATE(p2 == 0 && dataLength == sizeof(uint32_t), ERR_INVALID_REQUEST);

    uint32_t account_number = readUint32BE(dataBuffer);
    PublicKeyContext_t* context = &data_context.pk_context;
    get_public_key(account_number, context->public_key);
    if (p1 == P1_NON_CONFIRM) {
        *tx = set_result_get_public_key();
        THROW(SUCCESS);
    }

    if (p1 == P1_CONFIRM) {
        format_hex(context->public_key,
                   sizeof(context->public_key),
                   context->public_key_str,
                   sizeof(context->public_key_str));
        ui_display_public_key();
        *flags |= IO_ASYNCH_REPLY;
        return;
    }

    THROW(ERR_INVALID_REQUEST);
}
