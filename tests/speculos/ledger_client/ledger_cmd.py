from ast import List
from contextlib import contextmanager
import struct
from time import sleep
from typing import Tuple

from speculos.client import SpeculosClient, ApduException

from ledger_client.ledger_cmd_builder import LedgerCommandBuilder, InsType
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

        # response = FLAG (1) || MAJOR (1) || MINOR (1) || PATCH (1)
        assert len(response) == 3

        major, minor, patch = struct.unpack(
            "BBB",
            response
        )  # type: int, int, int

        return  major, minor, patch


    @contextmanager
    def get_public_key(self, bip32_path: str, result: List, display: bool = False) -> Tuple[bytes, bytes, bytes]:
        try:
            chunk: bytes = self.builder.get_public_key(bip32_path=bip32_path, display=display)
            with self.client.apdu_exchange_nowait(cla=chunk[0], ins=chunk[1],
                                                          p1=chunk[2], p2=chunk[3],
                                                          data=chunk[5:]) as exchange:
                yield exchange
                response: bytes = exchange.receive()
                
        except ApduException as error:
            raise DeviceException(error_code=error.sw, ins=InsType.INS_GET_PUBLIC_KEY)

        # response = pub_key_len (1) ||
        #            pub_key (var) ||
        #            chain_code_len (1) ||
        #            chain_code (var)
        offset: int = 0

        pub_key_len: int = response[offset]
        offset += 1

        uncompressed_addr_len: bytes = response[offset:offset + pub_key_len]
        offset += pub_key_len

        ledger_addr_len: int = response[offset]
        offset += 1

        ledger_addr: bytes = response[offset:offset + ledger_addr_len]
        offset += ledger_addr_len

        chain_code: bytes = response[offset:]

        assert len(response) == 1 + pub_key_len + 1 + ledger_addr_len + 32 # 32 -> chain_code_len
        
        result.append(uncompressed_addr_len)
        result.append(ledger_addr)
        result.append(chain_code)


