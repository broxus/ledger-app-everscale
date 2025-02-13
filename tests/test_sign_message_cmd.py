import pytest

from ragger.backend.interface import BackendInterface
from ragger.error import ExceptionRAPDU
from ragger.firmware import Firmware
from ragger.navigator import Navigator, NavInsID
from ragger.navigator.navigation_scenario import NavigateWithScenario

from application_client.everscale_command_sender import EverscaleCommandSender, Errors, WalletType
from application_client.everscale_response_unpacker import unpack_get_public_key_response, unpack_sign_tx_response
from utils import navigate_until_text_and_compare

# In this tests we check the behavior of the device when asked to sign a transaction


# In this test we send to the device a transaction to sign and validate it on screen
# The transaction is short and will be sent in one chunk
# We will ensure that the displayed information is correct by using screenshots comparison

# TODO: Add a valid raw message and a valid expected signature
@pytest.mark.active_test_scope
def test_sign_message(backend: BackendInterface, navigator: Navigator, default_screenshot_path: str, test_name: str) -> None:
    # Use the app interface instead of raw interface
    client = EverscaleCommandSender(backend)
    account_number = 0
    wallet_type = WalletType.WALLET_V3
    # First we need to get the public key of the device in order to build the transaction
    rapdu = client.get_public_key(account_number=account_number)

    # Message to sign
    payload = bytes.fromhex("00000000001111111111111111111111111111111111111111111111111111111111111111")

    # Send the sign device instruction.
    # As it requires on-screen validation, the function is asynchronous.
    # It will yield the result when the navigation is done
    with client.sign_message(payload=payload):
        # Validate the on-screen request by performing the navigation appropriate for this device
        navigate_until_text_and_compare(
            backend.firmware,
            navigator,
            "message.",
            default_screenshot_path,
            test_name
        )

    # The device as yielded the result, parse it and ensure that the signature is correct
    response = client.get_async_response().data
    _, der_sig, _ = unpack_sign_tx_response(response)
    assert der_sig.hex() == "d2edfcb178abad8df455f605d2887e6fed24ed1d2462e1fc216fad55f2e60fa372239ce950b7ab944a2eb80a2bcfaadfa02f870c6b3ed6df29fa9bc7419a730f"