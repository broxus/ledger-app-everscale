from ast import List
from contextlib import contextmanager
import struct
from time import sleep
from typing import Tuple

from speculos.client import SpeculosClient, ApduException

from ledger_client.ledger_cmd_builder import LedgerCommandBuilder, InsType, WalletType
from ledger_client.exception import DeviceException


class LedgerCommand:
    def __init__(self,
                 client: SpeculosClient,
                 debug: bool = False,
                 model: str = "nanos") -> None:
        self.client = client
        self.builder = LedgerCommandBuilder(debug=debug)
        self.debug = debug
        self.model = model
    
    def get_configuration(self) -> Tuple[ int, int, int]:
        try:
            response = self.client._apdu_exchange(
                self.builder.get_configuration()
            )  # type: int, bytes
        except ApduException as error:
            raise DeviceException(error_code=error.sw, ins=InsType.INS_GET_VERSION)
        # response = MAJOR (1) || MINOR (1) || PATCH (1)
        assert len(response) == 3
        major, minor, patch = struct.unpack(
            "BBB",
            response
        )  # type: int, int, int

        return  major, minor, patch

    def get_public_key(self, account: int, display: bool = False) -> str:
        try:
            response = self.client._apdu_exchange(
                self.builder.get_public_key(account=account, display=display)
            )  # type: bytes

        except ApduException as error:
            raise DeviceException(error_code=error.sw, ins=InsType.INS_GET_PUBLIC_KEY)

        # response = pub_key_len (1) ||
        #            pub_key (var)
        offset: int = 0

        pub_key_len: int = response[offset]

        offset += 1
        pubkey = response[offset:].hex()

        assert len(response) == 1 + pub_key_len

        return pubkey

    def get_address(self, account: int, wallet_type: int, display: bool = False) -> str:
        try:
            response = self.client._apdu_exchange(
                self.builder.get_address(account=account, wallet_type=wallet_type, display=display)
            )  # type: bytes
        except ApduException as error:
            raise DeviceException(error_code=error.sw, ins=InsType.INS_GET_ADDRESS)

        offset: int = 0

        addr_len: int = response[offset]

        offset += 1
        address = response[offset:].hex()

        assert len(response) == 1 + addr_len

        return address

    @contextmanager
    def sign_message(self, res: List, account: int, message: bytes) -> Tuple[bytes]:
        try:
            chunk: bytes = self.builder.sign_message(account=account, message=message)

            with self.client.apdu_exchange_nowait(cla=chunk[0], ins=chunk[1],
                                                  p1=chunk[2], p2=chunk[3],
                                                  data=chunk[5:]) as exchange:
                yield exchange

                response: bytes = exchange.receive()

        except ApduException as error:
            raise DeviceException(error_code=error.sw, ins=InsType.INS_SIGN)

        offset: int = 0
        signature_len: int = response[offset]
        assert signature_len == 64

        offset += 1
        signature = response[offset:]
        assert len(response) == 1 + signature_len

        res.append(signature)


    @contextmanager
    def sign_tx(self, res: List, account: int, wallet_type: int, decimals: int, ticker: str, meta: struct, data: bytes) -> Tuple[bytes]:
        try:
            chunk: bytes = self.builder.sign_tx(
                account=account, wallet_type=wallet_type, decimals=decimals, ticker=ticker, meta=meta, data=data)

            with self.client.apdu_exchange_nowait(cla=chunk[0], ins=chunk[1],
                                                  p1=chunk[2], p2=chunk[3],
                                                  data=chunk[5:]) as exchange:
                yield exchange

                response: bytes = exchange.receive()

        except ApduException as error:
            raise DeviceException(error_code=error.sw, ins=InsType.INS_SIGN_TX)

        offset: int = 0
        signature_len: int = response[offset]
        assert signature_len == 64

        offset += 1
        signature = response[offset:]
        assert len(response) == 1 + signature_len

        res.append(signature)

