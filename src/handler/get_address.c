#include "format.h"
#include "apdu_constants.h"
#include "globals.h"
#include "utils.h"
#include "contract.h"
#include "slice_data.h"
#include "byte_stream.h"
#include "response_setter.h"
#include "ui/display.h"
void handleGetAddress(uint8_t p1,
                      uint8_t p2,
                      uint8_t* dataBuffer,
                      uint16_t dataLength,
                      volatile unsigned int* flags,
                      volatile unsigned int* tx) {
    VALIDATE(p2 == 0 && dataLength == (sizeof(uint32_t) + sizeof(uint8_t)), ERR_INVALID_REQUEST);

    size_t offset = 0;

    uint32_t account_number = readUint32BE(dataBuffer + offset);
    offset += sizeof(account_number);

    uint8_t wallet_type = dataBuffer[offset];

    get_address(account_number, wallet_type, data_context.addr_context.address);

    if (p1 == P1_NON_CONFIRM) {
        *tx = set_result_get_address();
        THROW(SUCCESS);
    }
    if (p1 == P1_CONFIRM) {
        AddressContext_t* context = &data_context.addr_context;
        format_hex(context->address,
                   sizeof(context->address),
                   context->address_str,
                   sizeof(context->address_str));
        ui_display_address();
        *flags |= IO_ASYNCH_REPLY;
        return;
    }

    THROW(ERR_INVALID_REQUEST);
}
