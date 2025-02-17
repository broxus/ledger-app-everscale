from enum import IntEnum
from typing import Generator, List, Optional
from contextlib import contextmanager

from ragger.backend.interface import BackendInterface, RAPDU

MAX_APDU_LEN: int = 255

CLA: int = 0xE0


class P1(IntEnum):
    # Parameter 1 for first APDU number.
    P1_START = 0x00
    # Parameter 1 for maximum APDU number.
    P1_MAX = 0x03
    # Parameter 1 for screen confirmation for GET_PUBLIC_KEY.
    P1_CONFIRM = 0x01


class P2(IntEnum):
    # Parameter 2 for last APDU to receive.
    P2_LAST = 0x00
    # Parameter 2 for more APDU to receive.
    P2_MORE = 0x80


class InsType(IntEnum):
    GET_APP_CONFIGURATION = 0x01
    GET_PUBLIC_KEY = 0x02
    SIGN_MESSAGE = 0x03
    GET_ADDRESS = 0x04
    SIGN_TRANSACTION = 0x05


class WalletType(IntEnum):
    WALLET_V3 = 0
    EVER_WALLET = 1
    SAFE_MULTISIG_WALLET = 2
    SAFE_MULTISIG_WALLET_24H = 3
    SETCODE_MULTISIG_WALLET = 4
    BRIDGE_MULTISIG_WALLET = 5
    SURF_WALLET = 6
    MULTISIG_2 = 7
    MULTISIG_2_1 = 8


class Errors(IntEnum):
    SW_INVALID_DATA = 0x6B00
    SW_CELL_UNDERFLOW = 0x6B01
    SW_RANGE_CHECK_FAIL = 0x6B02
    SW_WRONG_LABEL = 0x6B03
    SW_INVALID_FLAG = 0x6B04
    SW_END_OF_STREAM = 0x6B05
    SW_SLICE_IS_EMPTY = 0x6B06
    SW_INVALID_KEY = 0x6B07
    SW_CELL_IS_EMPTY = 0x6B08
    SW_INVALID_HASH = 0x6B09
    SW_INVALID_CELL_INDEX = 0x6B10
    SW_INVALID_REQUEST = 0x6B11
    SW_INVALID_FUNCTION_ID = 0x6B12
    SW_INVALID_SRC_ADDRESS = 0x6B13
    SW_INVALID_WALLET_ID = 0x6B14
    SW_INVALID_WALLET_TYPE = 0x6B15
    SW_INVALID_TICKER_LENGTH = 0x6B16
    SW_DENY = 0x6985
    # Status Word from everscale app
    # SW_WRONG_P1P2              = 0x6A86
    # SW_WRONG_DATA_LENGTH       = 0x6A87
    # SW_INS_NOT_SUPPORTED       = 0x6D00
    # SW_CLA_NOT_SUPPORTED       = 0x6E00
    # SW_WRONG_RESPONSE_LENGTH   = 0xB000
    # SW_DISPLAY_BIP32_PATH_FAIL = 0xB001
    # SW_DISPLAY_ADDRESS_FAIL    = 0xB002
    # SW_DISPLAY_AMOUNT_FAIL     = 0xB003
    # SW_WRONG_TX_LENGTH         = 0xB004
    # SW_TX_PARSING_FAIL         = 0xB005
    # SW_TX_HASH_FAIL            = 0xB006
    # SW_BAD_STATE               = 0xB007
    # SW_SIGNATURE_FAIL          = 0xB008


def split_message(message: bytes, max_size: int) -> List[bytes]:
    return [message[x : x + max_size] for x in range(0, len(message), max_size)]


class EverscaleCommandSender:
    def __init__(self, backend: BackendInterface) -> None:
        self.backend = backend

    def get_app_and_version(self) -> RAPDU:
        return self.backend.exchange(
            cla=0xB0,  # specific CLA for BOLOS
            ins=0x01,  # specific INS for get_app_and_version
            p1=P1.P1_START,
            p2=P2.P2_LAST,
            data=b"",
        )

    def get_app_config(self) -> RAPDU:
        return self.backend.exchange(
            cla=CLA,
            ins=InsType.GET_APP_CONFIGURATION,
            p1=P1.P1_START,
            p2=P2.P2_LAST,
            data=b"",
        )

    def get_public_key(self, account_number: int) -> RAPDU:
        return self.backend.exchange(
            cla=CLA,
            ins=InsType.GET_PUBLIC_KEY,
            p1=P1.P1_START,
            p2=P2.P2_LAST,
            data=account_number.to_bytes(4, "big"),
        )

    @contextmanager
    def get_public_key_with_confirmation(
        self, account_number: int
    ) -> Generator[None, None, None]:
        with self.backend.exchange_async(
            cla=CLA,
            ins=InsType.GET_PUBLIC_KEY,
            p1=P1.P1_CONFIRM,
            p2=P2.P2_LAST,
            data=account_number.to_bytes(4, "big"),
        ) as response:
            yield response

    def get_address(self, account_number: int, wallet_type: WalletType) -> RAPDU:
        return self.backend.exchange(
            cla=CLA,
            ins=InsType.GET_ADDRESS,
            p1=P1.P1_START,
            p2=P2.P2_LAST,
            data=account_number.to_bytes(4, "big") + wallet_type.to_bytes(1, "big"),
        )

    @contextmanager
    def get_address_with_confirmation(
        self, account_number: int, wallet_type: WalletType
    ) -> Generator[None, None, None]:
        with self.backend.exchange_async(
            cla=CLA,
            ins=InsType.GET_ADDRESS,
            p1=P1.P1_CONFIRM,
            p2=P2.P2_LAST,
            data=account_number.to_bytes(4, "big") + wallet_type.to_bytes(1, "big"),
        ) as response:
            yield response

    @contextmanager
    def sign_message(self, payload: bytes) -> Generator[None, None, None]:
        with self.backend.exchange_async(
            cla=CLA,
            ins=InsType.SIGN_MESSAGE,
            p1=P1.P1_CONFIRM,
            p2=P2.P2_LAST,
            data=payload,
        ) as response:
            yield response

    @contextmanager
    def sign_tx(self, transaction: bytes) -> Generator[None, None, None]:
        with self.backend.exchange_async(
            cla=CLA,
            ins=InsType.SIGN_TRANSACTION,
            p1=P1.P1_CONFIRM,
            p2=P2.P2_LAST,
            data=transaction,
        ) as response:
            yield response

    def get_async_response(self) -> Optional[RAPDU]:
        return self.backend.last_async_response
