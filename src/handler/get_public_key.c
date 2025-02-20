#include "os.h"
#include "ux.h"
#include "format.h"
#include "read.h"
#include "buffer.h"
#include "parser.h"
#include "utils.h"
#include "apdu_constants.h"
#include "errors.h"
#include "ui/display.h"
#include "helper/send_response.h"

int handleGetPublicKey(buffer_t* cdata, bool display, volatile unsigned int* flags) {
    uint32_t account_number = read_u32_be(cdata->ptr, cdata->offset);
    PublicKeyContext_t* context = &data_context.pk_context;
    if (get_public_key(account_number, context->public_key) != 0) {
        return io_send_sw(ERR_GET_PUBLIC_KEY_FAILED);
    }

    if (display) {
        format_hex(context->public_key,
                   sizeof(context->public_key),
                   context->public_key_str,
                   sizeof(context->public_key_str));
        ui_display_public_key();
        *flags |= IO_ASYNCH_REPLY;
        return 0;
    }
    return helper_send_response_public_key();
}
