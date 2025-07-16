#include "format.h"
#include "read.h"
#include "buffer.h"
#include "apdu_constants.h"
#include "utils.h"
#include "errors.h"
#include "ui/display.h"
#include "helper/send_response.h"

static const char SIGN_MAGIC[] = {0xFF, 0xFF, 0xFF, 0xFF};

int handleSign(buffer_t* cdata, volatile unsigned int* flags) {
    SignContext_t* context = &data_context.sign_context;

    VALIDATE(cdata->size >= sizeof(context->account_number), ERR_INVALID_REQUEST);
    context->account_number = read_u32_be(cdata->ptr, cdata->offset);
    cdata->offset += sizeof(context->account_number);

    VALIDATE(cdata->size >= cdata->offset + TO_SIGN_LENGTH, ERR_INVALID_REQUEST);
    memcpy(context->to_sign, SIGN_MAGIC, SIGN_MAGIC_LENGTH);
    memcpy(context->to_sign + SIGN_MAGIC_LENGTH, cdata->ptr + cdata->offset, TO_SIGN_LENGTH);
    format_hex(context->to_sign,
               SIGN_MAGIC_LENGTH + TO_SIGN_LENGTH,
               context->to_sign_str,
               sizeof(context->to_sign_str));

    ui_display_sign();
    *flags |= IO_ASYNCH_REPLY;
    return 0;
}
