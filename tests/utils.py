from pathlib import Path
from typing import List
import re
from Crypto.Hash import keccak

from ecdsa.curves import SECP256k1
from ecdsa.keys import VerifyingKey
from ecdsa.util import sigdecode_der
from ragger.navigator import NavInsID

# Check if a signature of a given message is valid
def check_signature_validity(public_key: bytes, signature: bytes, message: bytes) -> bool:
    pk: VerifyingKey = VerifyingKey.from_string(
        public_key,
        curve=SECP256k1,
        hashfunc=None
    )
    # Compute message hash (keccak_256)
    k = keccak.new(digest_bits=256)
    k.update(message)
    message_hash = k.digest()

    return pk.verify_digest(signature=signature,
                     digest=message_hash,
                     sigdecode=sigdecode_der)


def verify_name(name: str) -> None:
    """Verify the app name, based on defines in Makefile

    Args:
        name (str): Name to be checked
    """

    name_str = ""
    lines = _read_makefile()
    name_re = re.compile(r"^APPNAME\s?=\s?\"?(?P<val>\w+)\"?", re.I)
    for line in lines:
        info = name_re.match(line)
        if info:
            dinfo = info.groupdict()
            name_str = dinfo["val"]
    assert name == name_str


def verify_version(version: str) -> None:
    """Verify the app version, based on defines in Makefile

    Args:
        Version (str): Version to be checked
    """

    vers_dict = {}
    vers_str = ""
    lines = _read_makefile()
    version_re = re.compile(r"^APPVERSION_(?P<part>\w)\s?=\s?(?P<val>\d*)", re.I)
    for line in lines:
        info = version_re.match(line)
        if info:
            dinfo = info.groupdict()
            vers_dict[dinfo["part"]] = dinfo["val"]
    try:
        vers_str = f"{vers_dict['M']}.{vers_dict['N']}.{vers_dict['P']}"
    except KeyError:
        pass
    assert version == vers_str


def _read_makefile() -> List[str]:
    """Read lines from the parent Makefile """

    parent = Path(__file__).parent.parent.resolve()
    makefile = f"{parent}/Makefile"
    with open(makefile, "r", encoding="utf-8") as f_p:
        lines = f_p.readlines()
    return lines


def navigate_until_text_and_compare(
    firmware,
    navigator,
    text: str,
    screenshot_path: str,
    test_name: str,
    screen_change_before_first_instruction: bool = True,
    screen_change_after_last_instruction: bool = True,
    nav_ins_confirm_instruction: List[NavInsID] = [NavInsID.USE_CASE_REVIEW_CONFIRM],
):
    """Navigate through device screens until specified text is found and compare screenshots.

    This function handles navigation through device screens differently based on the device type (Stax/Flex vs others).
    It will navigate through screens until the specified text is found, taking screenshots for comparison along the way.

    Args:
        firmware: The firmware object containing device information
        navigator: The navigator object used to control device navigation
        text: The text string to search for on device screens
        screenshot_path: Path where screenshot comparison files will be saved
        test_name: The name of the test that is being run
        screen_change_before_first_instruction: Whether to wait for screen change before first instruction
        screen_change_after_last_instruction: Whether to wait for screen change after last instruction
    Returns:
        None

    Note:
        For Stax/Flex devices:
        - Uses swipe left gesture for navigation
        - Uses review confirm for confirmation
        For other devices:
        - Uses right click for navigation
        - Uses both click for confirmation
    """
    if firmware.device.startswith(("stax", "flex")):
        go_right_instruction = NavInsID.SWIPE_CENTER_TO_LEFT
        confirm_instructions = nav_ins_confirm_instruction
    else:
        go_right_instruction = NavInsID.RIGHT_CLICK
        confirm_instructions = [NavInsID.BOTH_CLICK]

    navigator.navigate_until_text_and_compare(
        go_right_instruction,
        confirm_instructions,
        text,
        screenshot_path,
        test_name,
        300,
        screen_change_before_first_instruction,
        screen_change_after_last_instruction,
    )
