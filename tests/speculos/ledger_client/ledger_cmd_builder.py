import enum
import logging
import struct
from typing import List, Tuple, Union, Iterator, cast

from ledger_client.transaction import EIP712, PersonalTransaction, Transaction
#from ethereum_client.plugin import ERC20Information, Plugin
from ledger_client.utils import packed_bip32_path_from_string

MAX_APDU_LEN: int = 255

def chunked(size, source):
    for i in range(0, len(source), size):
        yield source[i:i+size]

def chunkify(data: bytes, chunk_len: int) -> Iterator[Tuple[bool, bytes]]:
    size: int = len(data)

    if size <= chunk_len:
        yield True, data
        return

    chunk: int = size // chunk_len
    remaining: int = size % chunk_len
    offset: int = 0

    for i in range(chunk):
        yield False, data[offset:offset + chunk_len]
        offset += chunk_len

    if remaining:
        yield True, data[offset:]

class InsType(enum.IntEnum):
    INS_GET_PUBLIC_KEY          = 0x02
    INS_GET_CONFIGURATION       = 0x01
    INS_SIGN                    = 0x03
    INS_GET_ADDRESS             = 0x04
    INS_SIGN_TX                 = 0x05


class WalletType(enum.IntEnum):
    WalletV3 = 0,
    EverWallet = 1,
    SafeMultisig = 2,
    SafeMultisig24h = 3,
    SetcodeMultisig = 4,
    BridgeMultisig = 5,
    Surf = 6,
    Multisig2 = 7,
    Multisig2_1 = 8,


class LedgerCommandBuilder:
    """APDU command builder for the Boilerplate application.

    Parameters
    ----------
    debug: bool
        Whether you want to see logging or not.

    Attributes
    ----------
    debug: bool
        Whether you want to see logging or not.

    """
    CLA: int = 0xE0

    def __init__(self, debug: bool = False):
        """Init constructor."""
        self.debug = debug

    def serialize(self,
                  cla: int,
                  ins: Union[int, enum.IntEnum],
                  p1: int = 0,
                  p2: int = 0,
                  cdata: bytes = b"") -> bytes:
        """Serialize the whole APDU command (header + data).

        Parameters
        ----------
        cla : int
            Instruction class: CLA (1 byte)
        ins : Union[int, IntEnum]
            Instruction code: INS (1 byte)
        p1 : int
            Instruction parameter 1: P1 (1 byte).
        p2 : int
            Instruction parameter 2: P2 (1 byte).
        cdata : bytes
            Bytes of command data.

        Returns
        -------
        bytes
            Bytes of a complete APDU command.

        """
        ins = cast(int, ins.value) if isinstance(ins, enum.IntEnum) else cast(int, ins)

        header: bytes = struct.pack("BBBBB",
                                    cla,
                                    ins,
                                    p1,
                                    p2,
                                    len(cdata))  # add Lc to APDU header

        if self.debug:
            logging.info("header: %s", header.hex())
            logging.info("cdata:  %s", cdata.hex())

        print("serialize INS=", ins)
        print("serialize DATA= ", cdata.hex())
        return header + cdata

    def get_configuration(self) -> bytes:
        """Command builder for GET_CONFIGURATON

        Returns
        -------
        bytes
            APDU command for GET_CONFIGURATON

        """
        return self.serialize(cla=self.CLA,
                              ins=InsType.INS_GET_CONFIGURATION,
                              p1=0x00,
                              p2=0x00,
                              cdata=b"")


    def get_public_key(self, account: int, display: bool = False) -> bytes:
        """Command builder for GET_PUBLIC_KEY.

        Parameters
        ----------
        bip32_path: str
            String representation of BIP32 path.
        display : bool
            Whether you want to display the address on the device.

        Returns
        -------
        bytes
            APDU command for GET_PUBLIC_KEY.

        """
        cdata = account.to_bytes(4, byteorder='big')
        print("cdata= ", cdata)
        return self.serialize(cla=self.CLA,
                              ins=InsType.INS_GET_PUBLIC_KEY,
                              p1=0x01 if display else 0x00,
                              p2=0x00,
                              cdata=cdata)

    def get_address(self, account: int, wallet_type: int, display: bool) -> bytes:
        """Command builder for GET_ADDRESS.

        Parameters
        ----------
        account: int
            Account identifier.
        wallet_type: int
            Wallet type.
        display : bool
            Whether you want to display the address on the device.

        Returns
        -------
        bytes
            APDU command for GET_ADDRESS.

        """

        #account_num = account.to_bytes(4, byteorder='big')

        #wallet = wallet_type.to_bytes(1, byteorder='big')

        cdata = (account.to_bytes(4, byteorder='big') + wallet_type.to_bytes(1, byteorder='big'))
        #cdata = (account_num + wallet)

        return self.serialize(cla=self.CLA,
                          ins=InsType.INS_GET_ADDRESS,
                          p1=0x01 if display else 0x00,
                          p2=0x00,
                          cdata=cdata)

    def sign_message(self, account: int, message: bytes) -> bytes:
        """Command builder for SIGN_MESSAGE.

        Parameters
        ----------
        account: int
            Account identifier.
        message: str
            Message hash.
        display : bool
            Whether you want to display the address on the device.

        Returns
        -------
        bytes
            APDU command for SIGN_MESSAGE.

        """

        cdata = (account.to_bytes(4, byteorder='big') + message)

        return self.serialize(cla=self.CLA,
                              ins=InsType.INS_SIGN,
                              p1=0x01,
                              p2=0x00,
                              cdata=cdata)

    def sign_tx(self, account: int, wallet_type: int, decimals: int, ticker: str, meta: struct, data: bytes) -> bytes:
        """Command builder for INS_SIGN_TX.

        Parameters
        ----------
        bip32_path : str
            String representation of BIP32 path.
        transaction : Transaction
            Representation of the transaction to be signed.

        Yields
        -------
        bytes
            APDU command chunk for INS_SIGN_TX.

        """
        meta_data = 0
        cdata = (account.to_bytes(4, byteorder='big') + wallet_type.to_bytes(1, byteorder='big')
                 + decimals.to_bytes(1, byteorder='big') + len(ticker).to_bytes(1, byteorder='big')
                 + bytes(ticker, 'UTF-8') + meta_data.to_bytes(1, byteorder='big') + data)

        return self.serialize(cla=self.CLA,
                              ins=InsType.INS_SIGN_TX,
                              p1=0x01,
                              p2=0x00,
                              cdata=cdata)

