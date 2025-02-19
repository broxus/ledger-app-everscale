#include "format.h"
#include "read.h"
#include "apdu_constants.h"
#include "globals.h"
#include "utils.h"
#include "contract.h"
#include "slice_data.h"
#include "byte_stream.h"
#include "ui/display.h"
#include "errors.h"
#include "io.h"
#include "helper/send_response.h"

int handleGetAddress(buffer_t* cdata, bool display, volatile unsigned int* flags) {
    uint32_t account_number = read_u32_be(cdata->ptr, cdata->offset);
    cdata->offset += sizeof(account_number);

    uint8_t wallet_type = cdata->ptr[cdata->offset++];

    AddressContext_t* context = &data_context.addr_context;
    get_address(account_number, wallet_type, context->address);

    if (display) {
        format_hex(context->address,
                   sizeof(context->address),
                   context->address_str,
                   sizeof(context->address_str));
        ui_display_address();
        *flags |= IO_ASYNCH_REPLY;
        return 0;
    }
    return helper_send_response_address();
}
