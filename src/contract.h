#ifndef _CONTRACT_H_
#define _CONTRACT_H_

#include "cell.h"

#include <stdint.h>
#include <stdbool.h>

#define SIGN_TRANSACTION_FLOW_ERROR    -1
#define SIGN_TRANSACTION_FLOW_TRANSFER 0
#define SIGN_TRANSACTION_FLOW_DEPLOY   1
#define SIGN_TRANSACTION_FLOW_CONFIRM  2
#define SIGN_TRANSACTION_FLOW_BURN     3

enum {
    WALLET_V3 = 0,
    EVER_WALLET = 1,
    SAFE_MULTISIG_WALLET = 2,
    SAFE_MULTISIG_WALLET_24H = 3,
    SETCODE_MULTISIG_WALLET = 4,
    BRIDGE_MULTISIG_WALLET = 5,
    SURF_WALLET = 6,
    MULTISIG_2 = 7,
    MULTISIG_2_1 = 8,
};

enum {
    MULTISIG_DEPLOY_TRANSACTION = 1813932348,
    MULTISIG_SEND_TRANSACTION = 1290691692,
    MULTISIG_SUBMIT_TRANSACTION = 320701133,
    MULTISIG_CONFIRM_TRANSACTION = 447168749,

    MULTISIG2_DEPLOY_TRANSACTION = 733015951,
    MULTISIG2_SUBMIT_TRANSACTION = 1428000117,
};

enum {
    TOKEN_BURN = 1445284013,                // Burn
    TOKEN_TRANSFER = 1944199491,            // Deploy
    TOKEN_TRANSFER_TO_WALLET = 1185535980,  // Ordinary transfer
};

enum { NORMAL_FLAG = 3, ALL_BALANCE_FLAG = 128, ALL_BALANCE_AND_DELETE_FLAG = 160 };

struct ByteStream_t;
void deserialize_cells_tree(struct ByteStream_t* src);
void get_address(const uint32_t account_number, uint8_t wallet_type, uint8_t* address);

#endif
