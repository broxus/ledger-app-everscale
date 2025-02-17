import pytest

from ragger.bip import calculate_public_key_and_chaincode, CurveChoice
from ragger.error import ExceptionRAPDU
from ragger.backend.interface import BackendInterface
from ragger.navigator.navigation_scenario import NavigateWithScenario

from application_client.everscale_command_sender import EverscaleCommandSender, Errors, WalletType
from application_client.everscale_response_unpacker import unpack_get_address_response

HARDENED_OFFSET = 0x80000000
PATH_PREFIX = "44'/396'/"
PATH_SUFFIX = "'/0'/0'"
    # 9 types of wallets
EXPECTED_ADDRESSES = [
    '7571b498e3fed7a0fffbe21377e50548c92da4a04842e1b163547d3e8980cf64', '522a4cc774797f537c41b4853a9b83f359fe2802a5cf3bd9f31240c495e82358', 'ae990783e06a196ab03029fe4517dda0ea318c091cd49ff51cc0a40369b0aff5', '98135fb68e91833e810122abfe00ff3b38c1d555bf773741f869dea8b87fb72d', '23e76dee54e3f715e11cf374177e786878814ad2c7ac6e38bc06515efdb5fab3', '0806dbe6c4581af1165879fd196d3e02404029540e818921edfedbffb46d3c65', 'ec03eb7af13d70083b9f3c8202b0321ede255edc624292c531106f50d9f005b3', '9760a1b7393cdbb389205f72ebf4d7e362b06b419fdac9aeccd83bf39ce0d05a', '1de569744cf341d8e2e35996f23cdf5d5f9226c1400c98100f480d89e70c6bcf'
]

# In this test we check that the GET_ADDRESS works in non-confirmation mode
@pytest.mark.active_test_scope
def test_get_address_all_types_no_confirm(backend: BackendInterface) -> None:
    temp = []
    for wallet_type in range(9):
        client = EverscaleCommandSender(backend)
        response = client.get_address(account_number=0, wallet_type=wallet_type).data
        _, address = unpack_get_address_response(response)

        assert address.hex() == EXPECTED_ADDRESSES[wallet_type], f"Error with wallet_type: {wallet_type}, expected: {EXPECTED_ADDRESSES[wallet_type]}, but got {address.hex()}"


# In this test we check that the GET_ADDRESS works in confirmation mode
@pytest.mark.active_test_scope
def test_get_address_wallet_v3_confirm_accepted(backend: BackendInterface, scenario_navigator: NavigateWithScenario) -> None:
    client = EverscaleCommandSender(backend)
    account_number = 0
    wallet_type = WalletType.WALLET_V3
    with client.get_address_with_confirmation(account_number=account_number, wallet_type=wallet_type):
        scenario_navigator.address_review_approve()

    response = client.get_async_response().data
    _, address = unpack_get_address_response(response)

    assert address.hex() == EXPECTED_ADDRESSES[wallet_type], f"Error with wallet_type: {wallet_type}, expected: {EXPECTED_ADDRESSES[wallet_type]}, but got {address.hex()}"


# In this test we check that the GET_ADDRESS in confirmation mode replies an error if the user refuses
@pytest.mark.active_test_scope
def test_get_address_wallet_v3_confirm_refused(backend: BackendInterface, scenario_navigator: NavigateWithScenario) -> None:
    client = EverscaleCommandSender(backend)
    account_number = 0
    wallet_type = WalletType.WALLET_V3

    with pytest.raises(ExceptionRAPDU) as e:
        with client.get_address_with_confirmation(account_number=account_number, wallet_type=wallet_type):
            scenario_navigator.address_review_reject()

    # Assert that we have received a refusal
    assert e.value.status == Errors.SW_DENY
    assert len(e.value.data) == 0





