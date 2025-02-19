#ifdef HAVE_NBGL
#include "display.h"
#include "contract.h"
#include "ui/action/validate.h"
#include "ui/menu.h"
static void review_choice_address(bool choice) {
    // Answer, display a status page and go back to main
    validate_address(choice);
    if (choice) {
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_VERIFIED, ui_main_menu);
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_ADDRESS_REJECTED, ui_main_menu);
    }
}

// TODO: Implement this
void ui_display_address() {
    nbgl_useCaseAddressReview(data_context.addr_context.address_str,
                              NULL,
                              &C_app_everscale_40px,
                              "Verify Address",
                              NULL,
                              review_choice_address);
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