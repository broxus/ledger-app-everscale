#ifdef HAVE_BAGL
#include "display.h"
#include "response_setter.h"
#include "contract.h"
// Screens and flows
UX_STEP_NOCB(ux_display_address_flow_1_step,
             pnn,
             {
                 &C_icon_eye,
                 "Verify",
                 "address",
             });
UX_STEP_NOCB(ux_display_address_flow_2_step,
             bnnn_paging,
             {
                 .title = "Address",
                 .text = data_context.addr_context.address_str,
             });
UX_STEP_CB(ux_display_address_flow_3_step,
           pb,
           send_response(0, false),
           {
               &C_icon_crossmark,
               "Reject",
           });
UX_STEP_CB(ux_display_address_flow_4_step,
           pb,
           send_response(set_result_get_address(), true),
           {
               &C_icon_validate_14,
               "Approve",
           });

UX_FLOW(ux_display_address_flow,
        &ux_display_address_flow_1_step,
        &ux_display_address_flow_2_step,
        &ux_display_address_flow_3_step,
        &ux_display_address_flow_4_step);

UX_STEP_NOCB(ux_display_public_flow_1_step,
             bnnn_paging,
             {
                 .title = "Public key",
                 .text = data_context.pk_context.public_key_str,
             });
UX_STEP_CB(ux_display_public_flow_2_step,
           pb,
           send_response(0, false),
           {
               &C_icon_crossmark,
               "Reject",
           });
UX_STEP_CB(ux_display_public_flow_3_step,
           pb,
           send_response(set_result_get_public_key(), true),
           {
               &C_icon_validate_14,
               "Approve",
           });

UX_FLOW(ux_display_public_flow,
        &ux_display_public_flow_1_step,
        &ux_display_public_flow_2_step,
        &ux_display_public_flow_3_step);

UX_STEP_NOCB(ux_sign_flow_1_step,
             pnn,
             {
                 &C_icon_certificate,
                 "Sign",
                 "message",
             });
UX_STEP_NOCB(ux_sign_flow_2_step,
             bnnn_paging,
             {
                 .title = "Message",
                 .text = data_context.sign_context.to_sign_str,
             });
UX_STEP_CB(ux_sign_flow_3_step,
           pbb,
           send_response(0, false),
           {
               &C_icon_crossmark,
               "Cancel",
               "signature",
           });
UX_STEP_CB(ux_sign_flow_4_step,
           pbb,
           send_response(set_result_sign(), true),
           {
               &C_icon_validate_14,
               "Sign",
               "message.",
           });

UX_FLOW(ux_sign_flow,
        &ux_sign_flow_1_step,
        &ux_sign_flow_2_step,
        &ux_sign_flow_3_step,
        &ux_sign_flow_4_step);

UX_STEP_NOCB(ux_sign_transaction_intro,
             pnn,
             {
                 &C_icon_eye,
                 "Review",
                 "transaction",
             });
UX_STEP_NOCB(ux_sign_transaction_burn, bnnn_paging, {.title = "Action", .text = "Burn"});
UX_STEP_NOCB(ux_sign_transaction_deploy, bnnn_paging, {.title = "Action", .text = "Deploy"});
UX_STEP_NOCB(ux_sign_transaction_confirm, bnnn_paging, {.title = "Action", .text = "Confirm"});
UX_STEP_NOCB(ux_sign_transaction_transfer, bnnn_paging, {.title = "Action", .text = "Transfer"});
UX_STEP_NOCB(ux_sign_transaction_amount,
             bnnn_paging,
             {
                 .title = "Amount",
                 .text = data_context.sign_tr_context.amount_str,
             });
UX_STEP_NOCB(ux_sign_transaction_address,
             bnnn_paging,
             {
                 .title = "Address",
                 .text = data_context.sign_tr_context.address_str,
             });
UX_STEP_NOCB(ux_sign_transaction_transaction_id,
             bnnn_paging,
             {
                 .title = "Transaction id",
                 .text = data_context.sign_tr_context.transaction_id_str,
             });
UX_STEP_CB(ux_sign_transaction_accept,
           pbb,
           send_response(set_result_sign_transaction(), true),
           {
               &C_icon_validate_14,
               "Accept",
               "and send",
           });
UX_STEP_CB(ux_sign_transaction_reject,
           pb,
           send_response(0, false),
           {
               &C_icon_crossmark,
               "Reject",
           });

UX_FLOW(ux_sign_transaction_burn_flow,
        &ux_sign_transaction_intro,
        &ux_sign_transaction_burn,
        &ux_sign_transaction_amount,
        &ux_sign_transaction_accept,
        &ux_sign_transaction_reject);

UX_FLOW(ux_sign_transaction_deploy_flow,
        &ux_sign_transaction_intro,
        &ux_sign_transaction_deploy,
        &ux_sign_transaction_address,
        &ux_sign_transaction_accept,
        &ux_sign_transaction_reject);

UX_FLOW(ux_sign_transaction_confirm_flow,
        &ux_sign_transaction_intro,
        &ux_sign_transaction_confirm,
        &ux_sign_transaction_transaction_id,
        &ux_sign_transaction_accept,
        &ux_sign_transaction_reject);

UX_FLOW(ux_sign_transaction_transfer_flow,
        &ux_sign_transaction_intro,
        &ux_sign_transaction_transfer,
        &ux_sign_transaction_amount,
        &ux_sign_transaction_address,
        &ux_sign_transaction_accept,
        &ux_sign_transaction_reject);

// Display functions
void ui_display_address() {
    ux_flow_init(0, ux_display_address_flow, NULL);
}

void ui_display_public_key() {
    ux_flow_init(0, ux_display_public_flow, NULL);
}

void ui_display_sign() {
    ux_flow_init(0, ux_sign_flow, NULL);
}

void ui_display_sign_transaction(int flow) {
    switch (flow) {
        case SIGN_TRANSACTION_FLOW_TRANSFER:
            ux_flow_init(0, ux_sign_transaction_transfer_flow, NULL);
            break;
        case SIGN_TRANSACTION_FLOW_DEPLOY:
            ux_flow_init(0, ux_sign_transaction_deploy_flow, NULL);
            break;
        case SIGN_TRANSACTION_FLOW_CONFIRM:
            ux_flow_init(0, ux_sign_transaction_confirm_flow, NULL);
            break;
        case SIGN_TRANSACTION_FLOW_BURN:
            ux_flow_init(0, ux_sign_transaction_burn_flow, NULL);
            break;
        default:
            THROW(ERR_INVALID_REQUEST);
    }
}
#endif