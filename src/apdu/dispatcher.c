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

#include <stdint.h>
#include <stdbool.h>
#include "parser.h"

#include "buffer.h"
#include "io.h"
#include "ledger_assert.h"
#include "apdu_constants.h"
#include "dispatcher.h"
#include "globals.h"
#include "handler/get_app_configuration.h"
#include "handler/get_public_key.h"
#include "handler/sign_transaction.h"
#include "handler/get_address.h"
#include "handler/sign.h"

int apdu_dispatcher(const command_t* cmd, volatile unsigned int* flags) {
    LEDGER_ASSERT(cmd != NULL, "NULL cmd");

    if (cmd->cla != CLA) {
        return io_send_sw(ERR_CLA_NOT_SUPPORTED);
    }

    buffer_t buf = {0};

    switch (cmd->ins) {
        case INS_GET_APP_CONFIGURATION:
            if (cmd->p1 != 0 || cmd->p2 != 0) {
                return io_send_sw(ERR_WRONG_P1P2);
            }
            if (cmd->lc != 0) {
                return io_send_sw(ERR_WRONG_DATA_LENGTH);
            }

            return handleGetAppConfiguration();
        case INS_GET_PUBLIC_KEY:
            if (cmd->p1 > 1 || cmd->p2 > 0) {
                return io_send_sw(ERR_WRONG_P1P2);
            }
            if (cmd->lc != sizeof(uint32_t)) {
                return io_send_sw(ERR_WRONG_DATA_LENGTH);
            }

            if (!cmd->data) {
                return io_send_sw(ERR_NO_DATA);
            }

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;

            return handleGetPublicKey(&buf, (bool) cmd->p1, flags);
        case INS_GET_ADDRESS:
            if (cmd->p1 > 1 || cmd->p2 > 0) {
                return io_send_sw(ERR_WRONG_P1P2);
            }
            if (cmd->lc != sizeof(uint32_t) + sizeof(uint8_t)) {
                return io_send_sw(ERR_WRONG_DATA_LENGTH);
            }

            if (!cmd->data) {
                return io_send_sw(ERR_NO_DATA);
            }

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;

            return handleGetAddress(&buf, (bool) cmd->p1, flags);
        case INS_SIGN:
            if (cmd->p1 != 1 || cmd->p2 > 0) {
                return io_send_sw(ERR_WRONG_P1P2);
            }
            if (cmd->lc == 0) {
                return io_send_sw(ERR_WRONG_DATA_LENGTH);
            }

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;

            return handleSign(&buf, flags);

        case INS_SIGN_TRANSACTION:
            if (cmd->p1 != P1_CONFIRM || cmd->p2 > 3) {
                return io_send_sw(ERR_WRONG_P1P2);
            }

            if (!cmd->data) {
                return io_send_sw(ERR_NO_DATA);
            }
            // P2_MORE is a signal for more apdu to receive in current chunk;
            // P2_EXTEND is a signal for extended buffer and can't be in first chunk;
            // P2_MORE && !P2_EXTEND = first chunk;
            // P2_EXTEND && !P2_MORE = last chunk;
            // P2_EXTEND && P2_MORE = ordinary request without chunks;

            // P2_EXTEND is set to signal that this APDU buffer extends, rather
            // than replaces, the current message buffer
            bool more = (bool) (cmd->p2 & P2_MORE);
            bool first_data_chunk = !(cmd->p2 & P2_EXTEND);

            buf.ptr = cmd->data;
            buf.size = cmd->lc;
            buf.offset = 0;

            return handleSignTransaction(&buf, first_data_chunk, more, flags);
        default:
            return io_send_sw(ERR_INS_NOT_SUPPORTED);
    }
}
