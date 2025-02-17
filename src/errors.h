#ifndef _ERRORS_H_
#define _ERRORS_H_

enum {
    SUCCESS                 = 0x9000,
    ERR_INVALID_DATA        = 0x6B00,
    ERR_CELL_UNDERFLOW      = 0x6B01,
    ERR_RANGE_CHECK         = 0x6B02,
    ERR_WRONG_LABEL         = 0x6B03,
    ERR_INVALID_FLAG        = 0x6B04,
    ERR_END_OF_STREAM       = 0x6B05,
    ERR_SLICE_IS_EMPTY      = 0x6B06,
    ERR_INVALID_KEY         = 0x6B07,
    ERR_CELL_IS_EMPTY       = 0x6B08,
    ERR_INVALID_HASH        = 0x6B09,
    ERR_INVALID_CELL_INDEX  = 0x6B10,
    ERR_INVALID_REQUEST     = 0x6B11,
    ERR_INVALID_FUNCTION_ID = 0x6B12,
    ERR_INVALID_SRC_ADDRESS = 0x6B13,
    ERR_INVALID_WALLET_ID   = 0x6B14,
    ERR_INVALID_WALLET_TYPE = 0x6B15,
    ERR_TICKER_LENGTH       = 0x6B16,
    ERR_INVALID_CELL        = 0x6B17,
    ERR_INVALID_CONTRACT    = 0x6B18,
    ERR_INVALID_MESSAGE     = 0x6B19,
    ERR_SIGNING_FAILED      = 0x6B20,
    ERR_GENERATE_PAIR_FAILED = 0x6B21,
    ERR_INIT_PRIVATE_KEY_FAILED = 0x6B22,
    ERR_DERIVE_PATH_FAILED = 0x6B23,
};

#endif
