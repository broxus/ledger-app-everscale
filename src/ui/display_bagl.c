#ifdef HAVE_BAGL
#include "display.h"
#include "contract.h"
#include "ui/action/validate.h"
#include "ui/menu.h"

static action_validate_cb g_validate_callback;

static void ui_action_validate_pubkey(bool choice) {
    validate_pubkey(choice);
    ui_main_menu();
}

static void ui_action_validate_address(bool choice) {
    validate_address(choice);
    ui_main_menu();
}

static void ui_action_validate_message(bool choice) {
    validate_message(choice);
    ui_main_menu();
}

static void ui_action_validate_transaction(bool choice) {
    validate_transaction(choice);
    ui_main_menu();
}

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
UX_STEP_CB(ux_display_reject_step,
           pb,
           (*g_validate_callback)(false),
           {
               &C_icon_crossmark,
               "Reject",
           });
UX_STEP_CB(ux_display_approve_step,
           pb,
           (*g_validate_callback)(true),
           {
               &C_icon_validate_14,
               "Approve",
           });

UX_FLOW(ux_display_address_flow,
        &ux_display_address_flow_1_step,
        &ux_display_address_flow_2_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

UX_STEP_NOCB(ux_display_public_flow_1_step,
             pnn,
             {
                 &C_icon_eye,
                 "Verify",
                 "Public key",
             });
UX_STEP_NOCB(ux_display_public_flow_2_step,
             bnnn_paging,
             {
                 .title = "Public key",
                 .text = data_context.pk_context.public_key_str,
             });

UX_FLOW(ux_display_public_flow,
        &ux_display_public_flow_1_step,
        &ux_display_public_flow_2_step,
        &ux_display_approve_step,
        &ux_display_reject_step);

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
UX_STEP_CB(ux_sign_flow_4_step,
           pbb,
           (*g_validate_callback)(false),
           {
               &C_icon_crossmark,
               "Cancel",
               "signature",
           });
UX_STEP_CB(ux_sign_flow_3_step,
           pbb,
           (*g_validate_callback)(true),
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

UX_FLOW(ux_sign_transaction_burn_flow,
        &ux_sign_transaction_intro,
        &ux_sign_transaction_burn,
        &ux_sign_transaction_amount,
        &ux_display_approve_step,
        &ux_display_reject_step);

UX_FLOW(ux_sign_transaction_deploy_flow,
        &ux_sign_transaction_intro,
        &ux_sign_transaction_deploy,
        &ux_sign_transaction_address,
        &ux_display_approve_step,
        &ux_display_reject_step);

UX_FLOW(ux_sign_transaction_confirm_flow,
        &ux_sign_transaction_intro,
        &ux_sign_transaction_confirm,
        &ux_sign_transaction_transaction_id,
        &ux_display_approve_step,
        &ux_display_reject_step);

UX_FLOW(ux_sign_transaction_transfer_flow,
        &ux_sign_transaction_intro,
        &ux_sign_transaction_transfer,
        &ux_sign_transaction_amount,
        &ux_sign_transaction_address,
        &ux_display_approve_step,
        &ux_display_reject_step);

// Display functions
void ui_display_address() {
    g_validate_callback = &ui_action_validate_address;
    ux_flow_init(0, ux_display_address_flow, NULL);
}

void ui_display_public_key() {
    g_validate_callback = &ui_action_validate_pubkey;
    ux_flow_init(0, ux_display_public_flow, NULL);
}

void ui_display_sign() {
    g_validate_callback = &ui_action_validate_message;
    ux_flow_init(0, ux_sign_flow, NULL);
}

void ui_display_sign_transaction(int flow) {
    g_validate_callback = &ui_action_validate_transaction;
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