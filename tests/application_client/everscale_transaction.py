# from application_client.everscale_command_sender import WalletType

# class Transaction:
#     ADDRESS_LENGTH = 32
#     CHAIN_ID_LENGTH = 4

#     FLAG_WITH_WALLET_ID = 0x01
#     FLAG_WITH_WORKCHAIN_ID = 0x02
#     FLAG_WITH_ADDRESS = 0x04
#     FLAG_WITH_CHAIN_ID = 0x08

#     def __init__(self,
#                  decimals: int,
#                  ticker: str,
#                  message: bytes,
#                  current_wallet_type: WalletType,
#                  workchain_id: int | None = None,
#                  prepend_address: bytes | None = None,
#                  chain_id: bytes | None = None) -> None:
#         """
#         Construct a Transaction.

#         The metadata byte is deduced as follows:
#           - FLAG_WITH_WALLET_ID is set if a current_wallet_type is provided and
#             it differs from the origin_wallet_type.
#           - FLAG_WITH_WORKCHAIN_ID is set if workchain_id is provided.
#           - FLAG_WITH_ADDRESS is set if prepend_address is provided.
#           - FLAG_WITH_CHAIN_ID is set if chain_id is provided.

#         If current_wallet_type is None or equals origin_wallet_type, then that field is omitted
#         and on deserialization the origin_wallet_type will be used.
#         """
#         self.decimals = decimals
#         self.ticker = ticker
#         self.message = message
#         self.workchain_id = workchain_id
#         self.prepend_address = prepend_address
#         self.chain_id = chain_id

#         # Deduce metadata flags based on optional inputs.
#         metadata = 0
#         if current_wallet_type is not None:
#             metadata |= self.FLAG_WITH_WALLET_ID
#             self.current_wallet_type = current_wallet_type
#         else:
#             # Do not include the field; on the device, it will be set to origin_wallet_type.
#             self.current_wallet_type = None

#         if workchain_id is not None:
#             metadata |= self.FLAG_WITH_WORKCHAIN_ID

#         if prepend_address is not None:
#             if len(prepend_address) != self.ADDRESS_LENGTH:
#                 raise ValueError(f"prepend_address must be {self.ADDRESS_LENGTH} bytes")
#             metadata |= self.FLAG_WITH_ADDRESS

#         if chain_id is not None:
#             if len(chain_id) != self.CHAIN_ID_LENGTH:
#                 raise ValueError(f"chain_id must be {self.CHAIN_ID_LENGTH} bytes")
#             metadata |= self.FLAG_WITH_CHAIN_ID

#         self.metadata = metadata

#         # Nice-to-haves: check ticker length constraints.
#         ticker_len = len(ticker)
#         if ticker_len == 0 or ticker_len > 10:
#             raise ValueError("Ticker length must be between 1 and 10 bytes.")

#     def serialize(self) -> bytes:
#         """
#         Serialize the transaction into a byte-buffer with the following structure:

#         [decimals:1] [ticker_length:1] [ticker:N] [metadata:1]
#         [optional fields (if flagged)...] [message:remaining bytes]
#         """
#         result = bytearray()
#         # 1. Decimals (1 byte)
#         result.append(self.decimals)

#         # 2. Ticker: length (1 byte) followed by its ASCII bytes
#         ticker_bytes = self.ticker.encode("ascii")
#         ticker_len = len(ticker_bytes)
#         result.append(ticker_len)
#         result.extend(ticker_bytes)

#         # 3. Metadata (1 byte; deduced from optional parameters)
#         result.append(self.metadata)

#         # 4. Conditionally append optional fields based on metadata flags.
#         if self.metadata & self.FLAG_WITH_WALLET_ID:
#             # current_wallet_type is provided and differs from origin_wallet_type.
#             result.append(self.current_wallet_type)
#         # Workchain id.
#         if self.metadata & self.FLAG_WITH_WORKCHAIN_ID:
#             result.append(self.workchain_id)
#         # Prepend address.
#         if self.metadata & self.FLAG_WITH_ADDRESS:
#             result.extend(self.prepend_address)
#         # Chain id.
#         if self.metadata & self.FLAG_WITH_CHAIN_ID:
#             result.extend(self.chain_id)

#         # 5. Append the message (payload).
#         result.extend(self.message)

#         return bytes(result)

#     @classmethod
#     def from_bytes(cls, data: bytes) -> "Transaction":
#         """
#         Deserialize a byte-buffer into a Transaction object.

#         The parsing order in the buffer is:
#           decimals (1 byte) ->
#           ticker_length (1 byte) ->
#           ticker ->
#           metadata (1 byte) ->
#           [optional fields... based on metadata] ->
#           message (remaining bytes)

#         If the metadata does NOT include FLAG_WITH_WALLET_ID, then current_wallet_type
#         defaults to origin_wallet_type.
#         """
#         offset = 0

#         # Read decimals (1 byte)
#         if len(data) < offset + 1:
#             raise ValueError("Data too short for decimals")
#         decimals = data[offset]
#         offset += 1

#         # Read ticker: first its length (1 byte) then the ticker string
#         if len(data) < offset + 1:
#             raise ValueError("Data too short for ticker length")
#         ticker_len = data[offset]
#         offset += 1
#         if len(data) < offset + ticker_len:
#             raise ValueError("Data too short for ticker")
#         ticker = data[offset:offset+ticker_len].decode("ascii")
#         offset += ticker_len

#         # Read metadata (1 byte)
#         if len(data) < offset + 1:
#             raise ValueError("Data too short for metadata")
#         metadata = data[offset]
#         offset += 1

#         # Read optional fields based on metadata flags.
#         current_wallet_type = None
#         if metadata & cls.FLAG_WITH_WALLET_ID:
#             if len(data) < offset + 1:
#                 raise ValueError("Data too short for current_wallet_type")
#             current_wallet_type = data[offset]
#             offset += 1

#         workchain_id = None
#         if metadata & cls.FLAG_WITH_WORKCHAIN_ID:
#             if len(data) < offset + 1:
#                 raise ValueError("Data too short for workchain_id")
#             workchain_id = data[offset]
#             offset += 1

#         prepend_address = None
#         if metadata & cls.FLAG_WITH_ADDRESS:
#             if len(data) < offset + cls.ADDRESS_LENGTH:
#                 raise ValueError("Data too short for prepend_address")
#             prepend_address = data[offset:offset+cls.ADDRESS_LENGTH]
#             offset += cls.ADDRESS_LENGTH

#         chain_id = None
#         if metadata & cls.FLAG_WITH_CHAIN_ID:
#             if len(data) < offset + cls.CHAIN_ID_LENGTH:
#                 raise ValueError("Data too short for chain_id")
#             chain_id = data[offset:offset+cls.CHAIN_ID_LENGTH]
#             offset += cls.CHAIN_ID_LENGTH

#         # The remaining bytes are the message.
#         message = data[offset:]

#         return cls(
#             decimals=decimals,
#             ticker=ticker,
#             message=message,
#             current_wallet_type=current_wallet_type,
#             workchain_id=workchain_id,
#             prepend_address=prepend_address,
#             chain_id=chain_id
#         )
