#ifdef HAVE_NBGL
#include "display.h"
#include "response_setter.h"
#include "contract.h"

// TODO: Implement this
void ui_display_address() {
}

// TODO: Implement this
void ui_display_public_key() {
}

// TODO: Implement this
void ui_display_sign_transaction(int flow) {
    switch (flow) {
        case SIGN_TRANSACTION_FLOW_TRANSFER:
            break;
        case SIGN_TRANSACTION_FLOW_DEPLOY:
            break;
        case SIGN_TRANSACTION_FLOW_CONFIRM:
            break;
        case SIGN_TRANSACTION_FLOW_BURN:
            break;
        default:
            THROW(ERR_INVALID_REQUEST);
            break;
    }
}

// TODO: Implement this
void ui_display_sign() {
}

#endif