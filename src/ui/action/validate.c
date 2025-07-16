/*****************************************************************************
 *   Ledger App Everscale.
 *   (c) 2020 Ledger SAS.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/

#include "io.h"
#include "validate.h"
#include "errors.h"
#include "globals.h"
#include "utils.h"
#include "helper/send_response.h"

void validate_pubkey(bool choice) {
    if (choice) {
        helper_send_response_public_key();
    } else {
        io_send_sw(ERR_USER_REJECTED);
    }
}

void validate_address(bool choice) {
    if (choice) {
        helper_send_response_address();
    } else {
        io_send_sw(ERR_USER_REJECTED);
    }
}

static int crypto_sign_message(void) {
    cx_ecfp_private_key_t privateKey;
    SignContext_t* context = &data_context.sign_context;
    cx_err_t error;

    if (get_private_key(context->account_number, &privateKey) != 0) {
        return -1;
    }
    error = cx_eddsa_sign_no_throw(&privateKey,
                                   CX_SHA512,
                                   context->to_sign,
                                   SIGN_MAGIC_LENGTH + TO_SIGN_LENGTH,
                                   context->signature,
                                   SIGNATURE_LENGTH);
    if (error != CX_OK) {
        return -2;
    }

    explicit_bzero(&privateKey, sizeof(privateKey));
    PRINTF("Signature: %.*H\n", SIGNATURE_LENGTH, context->signature);
    return 0;
}

void validate_message(bool choice) {
    if (choice) {
        if (crypto_sign_message() != 0) {
            io_send_sw(ERR_SIGNING_FAILED);
        } else {
            helper_send_response_sign();
        }
    } else {
        io_send_sw(ERR_USER_REJECTED);
    }
}

static int crypto_sign_transaction(void) {
    cx_ecfp_private_key_t privateKey;
    SignTransactionContext_t* context = &data_context.sign_tr_context;
    cx_err_t error;

    if (get_private_key(context->account_number, &privateKey) != 0) {
        return -1;
    }
    if (!context->sign_with_chain_id) {
        error = cx_eddsa_sign_no_throw(&privateKey,
                                       CX_SHA512,
                                       context->to_sign,
                                       TO_SIGN_LENGTH,
                                       context->signature,
                                       SIGNATURE_LENGTH);
    } else {
        error = cx_eddsa_sign_no_throw(&privateKey,
                                       CX_SHA512,
                                       context->to_sign,
                                       CHAIN_ID_LENGTH + TO_SIGN_LENGTH,
                                       context->signature,
                                       SIGNATURE_LENGTH);
    }
    if (error != CX_OK) {
        return -2;
    }
    explicit_bzero(&privateKey, sizeof(privateKey));
    return 0;
}

void validate_transaction(bool choice) {
    if (choice) {
        if (crypto_sign_transaction() != 0) {
            io_send_sw(ERR_SIGNING_FAILED);
        } else {
            helper_send_response_sign_transaction();
        }
    } else {
        io_send_sw(ERR_USER_REJECTED);
    }
}