from ast import List
from contextlib import contextmanager
import struct
from time import sleep
from typing import Tuple

from speculos.client import SpeculosClient, ApduException

from ledger_client.ledger_cmd_builder import LedgerCommandBuilder, InsType, WalletType
from ledger_client.exception import DeviceException
#from ledger_client.transaction import EIP712, PersonalTransaction, Transaction
#from ledger_client.plugin import ERC20Information, Plugin
#from ledger_client.utils import parse_sign_response


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


    @contextmanager
    def get_public_key(self, account: int, result: List, display: bool = False) -> Tuple[bytes, bytes, bytes]:
        try:
            chunk: bytes = self.builder.get_public_key(account=account, display=display)

            with self.client.apdu_exchange_nowait(cla=chunk[0], ins=chunk[1],
                                                          p1=chunk[2], p2=chunk[3],
                                                          data=chunk[5:]) as exchange:
                yield exchange

                response: bytes = exchange.receive()

        except ApduException as error:
            raise DeviceException(error_code=error.sw, ins=InsType.INS_GET_PUBLIC_KEY)

        # response = pub_key_len (1) ||
        #            pub_key (var)
        offset: int = 0

        pub_key_len: int = response[offset]
        print("PUBKEY LEN=", pub_key_len)

        offset += 1
        pubkey = response[offset:].hex()
        print("PUBKEY =", pubkey)
        #uncompressed_addr_len: bytes = response[offset:offset + pub_key_len]
        #offset += pub_key_len

        assert len(response) == 1 + pub_key_len
        
        result.append(pubkey)

    def get_address(self, account: int, wallet_type: int, display: bool = False) -> str:
        try:
            print("WALLET", wallet_type)
            response = self.client._apdu_exchange(
                self.builder.get_address(account=account, wallet_type=wallet_type, display=display)
            )  # type: bytes
        except ApduException as error:
            raise DeviceException(error_code=error.sw, ins=InsType.INS_GET_ADDRESS)

        offset: int = 0

        addr_len: int = response[offset]
        print("ADDR LEN=", addr_len)

        offset += 1
        address = response[offset:].hex()
        print("ADDR =", address)

        assert len(response) == 1 + addr_len

        return address

    def sign_message(self, account: int, message: bytes) -> bytes:
        try:
            response = self.client._apdu_exchange(
                self.builder.sign_message(account=account, message=message)
            )  # type: bytes
        except ApduException as error:
            raise DeviceException(error_code=error.sw, ins=InsType.INS_SIGN)

        offset: int = 0
        signature_len: int = response[offset]
        assert signature_len == 64
        print("SIGN LEN=", signature_len)

        offset += 1
        signature = response[offset:]
        print("SIGN =", signature)
        #assert len(signature) == 64
        assert len(response) == 1 + signature_len

        return signature
