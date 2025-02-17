import pytest
from ragger.backend.interface import BackendInterface

from application_client.everscale_command_sender import EverscaleCommandSender
from application_client.everscale_response_unpacker import unpack_get_version_response

from utils import verify_version

# In this test we check the behavior of the device when asked to provide the app version
@pytest.mark.active_test_scope
def test_get_app_config(backend: BackendInterface) -> None:
    # Use the app interface instead of raw interface
    client = EverscaleCommandSender(backend)
    # Send the GET_VERSION instruction
    rapdu = client.get_app_config()
    # Use an helper to parse the response, assert the values
    MAJOR, MINOR, PATCH = unpack_get_version_response(rapdu.data)
    verify_version(f"{MAJOR}.{MINOR}.{PATCH}")
