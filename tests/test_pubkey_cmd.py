import pytest

from ragger.bip import calculate_public_key_and_chaincode, CurveChoice
from ragger.error import ExceptionRAPDU
from ragger.backend.interface import BackendInterface
from ragger.navigator import NavInsID
from ragger.navigator.navigation_scenario import NavigateWithScenario

from utils import navigate_until_text_and_compare

from application_client.everscale_command_sender import EverscaleCommandSender, Errors
from application_client.everscale_response_unpacker import unpack_get_public_key_response

HARDENED_OFFSET = 0x80000000
PATH_PREFIX = "44'/396'/"
PATH_SUFFIX = "'/0'/0'"

# In this test we check that the GET_PUBLIC_KEY works in non-confirmation mode
@pytest.mark.active_test_scope
def test_get_public_key_no_confirm(backend: BackendInterface) -> None:
    account_number_list = [
        0,
        1,
        911,
        255,
        2147483647
    ]
    for account_number in account_number_list:
        client = EverscaleCommandSender(backend)
        response = client.get_public_key(account_number=account_number).data
        _, public_key = unpack_get_public_key_response(response)

        ref_public_key, _ = calculate_public_key_and_chaincode(CurveChoice.Ed25519Slip, path=PATH_PREFIX + str(account_number | HARDENED_OFFSET) + PATH_SUFFIX)        
        assert "00" + public_key.hex() == ref_public_key


# In this test we check that the GET_PUBLIC_KEY works in confirmation mode
@pytest.mark.active_test_scope
def test_get_public_key_confirm_accepted(backend: BackendInterface, firmware, navigator, default_screenshot_path, test_name) -> None:
    client = EverscaleCommandSender(backend)
    account_number = 0
    with client.get_public_key_with_confirmation(account_number=account_number):
        navigate_until_text_and_compare(firmware, navigator, "Approve", default_screenshot_path, test_name, True, True, [NavInsID.USE_CASE_CHOICE_CONFIRM])

    response = client.get_async_response().data
    _, public_key  = unpack_get_public_key_response(response)

    ref_public_key, _ = calculate_public_key_and_chaincode(CurveChoice.Ed25519Slip, path=PATH_PREFIX + str(account_number | HARDENED_OFFSET) + PATH_SUFFIX)
    assert "00" + public_key.hex() == ref_public_key


# In this test we check that the GET_PUBLIC_KEY in confirmation mode replies an error if the user refuses
@pytest.mark.active_test_scope
def test_get_public_key_confirm_refused(backend: BackendInterface, firmware, navigator, default_screenshot_path, test_name, scenario_navigator) -> None:
    client = EverscaleCommandSender(backend)
    account_number = 0
    
    confirm_instructions = [NavInsID.USE_CASE_CHOICE_REJECT, NavInsID.USE_CASE_CHOICE_CONFIRM]

    with pytest.raises(ExceptionRAPDU) as e:
        with client.get_public_key_with_confirmation(account_number=account_number):
            navigate_until_text_and_compare(firmware, navigator, "Reject", default_screenshot_path, test_name, True, True, confirm_instructions)

        # with client.get_public_key_with_confirmation(account_number=account_number):
        #     scenario_navigator.address_review_reject()

    # Assert that we have received a refusal
    assert e.value.status == Errors.SW_DENY
    assert len(e.value.data) == 0
